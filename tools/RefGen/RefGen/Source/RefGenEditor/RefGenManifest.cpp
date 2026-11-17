// -run=RefGen -Manifest=<file.json>: the reference values of the generated content, as the engine sees it, for the
// parser tests (tests/Paks/<platform>/<version>.json). Run after generating and baking, before cooking:
//   {"engine": "5.8", "assets": {object path: class}, "since": {}, "values": {"level.actor": {map: {actor: [class, transform]}}}}
// Only what survives cooking: editor-only actors are left out, and World Partition maps list no actors (the cook moves
// them into generated cell packages).
#include "RefGenCommon.h"
#include "RefGenCatalog.h"
#include "Misc/FileHelper.h"

namespace
{
	FString Quote(const FString& Text)
	{
		FString Out = TEXT("\"");
		for (const TCHAR C : Text)
		{
			if (C == TEXT('"') || C == TEXT('\\'))
				Out.AppendChar(TEXT('\\'));
			Out.AppendChar(C);
		}
		return Out + TEXT("\"");
	}

	FString Join(const TArray<FString>& Lines)
	{
		FString Out;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
			Out += (Index ? TEXT(",\n") : TEXT("")) + Lines[Index];
		return Out;
	}

	bool IsLive(AActor* Actor)
	{
#if UE_AT_LEAST(5, 0)
		return IsValid(Actor);
#else
		return Actor && !Actor->IsPendingKill();
#endif
	}

	FString Number(double Value)
	{
		if (FMath::Abs(Value) < 5e-8)
			Value = 0; // no "-0"
		FString Text = FString::Printf(TEXT("%.7g"), Value);
		if (!Text.Contains(TEXT(".")) && !Text.Contains(TEXT("e")))
			Text += TEXT(".0");
		return Text;
	}

	FString Vector3(double X, double Y, double Z)
	{
		return FString::Printf(TEXT("[%s, %s, %s]"), *Number(X), *Number(Y), *Number(Z));
	}

	FString TransformJson(const FTransform& T)
	{
		const FVector L = T.GetLocation();
		const FQuat R = T.GetRotation();
		const FVector S = T.GetScale3D();
		return FString::Printf(TEXT("{\"r\": [%s, %s, %s, %s], \"s\": %s, \"t\": %s}"), *Number(R.X), *Number(R.Y), *Number(R.Z), *Number(R.W),
			*Vector3(S.X, S.Y, S.Z), *Vector3(L.X, L.Y, L.Z));
	}

	bool SurvivesCook(AActor* Actor)
	{
#if UE_AT_LEAST(4, 15)
		if (Actor->IsEditorOnly())
			return false;
#endif
		return Actor->NeedsLoadForClient() || Actor->NeedsLoadForServer();
	}

	FString ClassOf(const FAssetData& Data)
	{
#if UE_AT_LEAST(5, 1)
		return Data.AssetClassPath.GetAssetName().ToString();
#else
		return Data.AssetClass.ToString();
#endif
	}

	FString PathOf(const FAssetData& Data)
	{
#if UE_AT_LEAST(5, 1)
		return Data.GetObjectPathString();
#else
		return Data.ObjectPath.ToString();
#endif
	}
}

int32 RefGen::WriteManifest(const FString& File)
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(FName(TEXT("/Game")), Assets, true);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return PathOf(A) < PathOf(B); });

	TArray<FString> AssetLines;
	TArray<FString> MapLines;
	for (const FAssetData& Data : Assets)
	{
		const FString Path = PathOf(Data);
		const FString Package = Data.PackageName.ToString();
		if (Package.Contains(TEXT("/__ExternalActors__/")) || Package.Contains(TEXT("/__ExternalObjects__/")))
			continue; // World Partition actor packages: cooked into the generated cells, not on their own
		const FString Class = ClassOf(Data);
		if (Class == TEXT("BlueprintGeneratedClass") || Class == TEXT("WidgetBlueprintGeneratedClass") || Class == TEXT("AnimBlueprintGeneratedClass"))
			continue; // in-memory registry entries of loaded blueprints; the package's asset is the blueprint
		AssetLines.Add(FString::Printf(TEXT("    %s: %s"), *Quote(Path), *Quote(Class)));
		if (Class != TEXT("World"))
			continue;
		UWorld* World = LoadObject<UWorld>(nullptr, *Path);
		if (!World || !World->PersistentLevel)
		{
			Fail(FString::Printf(TEXT("manifest: load %s"), *Path));
			continue;
		}
#if UE_AT_LEAST(5, 0)
		if (World->IsPartitionedWorld())
			continue;
#endif
		TArray<FString> ActorLines;
		for (AActor* Actor : World->PersistentLevel->Actors)
			if (IsLive(Actor) && SurvivesCook(Actor))
				ActorLines.Add(FString::Printf(TEXT("        %s: [%s, %s]"), *Quote(Actor->GetName()), *Quote(Actor->GetClass()->GetPathName()),
					*TransformJson(Actor->ActorToWorld())));
		ActorLines.Sort();
		MapLines.Add(FString::Printf(TEXT("      %s: {\n%s\n      }"), *Quote(Path), *Join(ActorLines)));
		UE_LOG(LogRefGen, Display, TEXT("manifest: %s, %d actors"), *Path, ActorLines.Num());
	}

	const FString Json = FString::Printf(TEXT("{\n  \"engine\": \"%d.%d\",\n  \"assets\": {\n%s\n  },\n  \"since\": {},\n  \"values\": {\n    \"level.actor\": {\n%s\n    }\n  }\n}\n"),
		ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, *Join(AssetLines), *Join(MapLines));
	if (!FFileHelper::SaveStringToFile(Json, *File))
		Fail(FString::Printf(TEXT("manifest: write %s"), *File));
	UE_LOG(LogRefGen, Display, TEXT("manifest: %d assets, %d maps -> %s"), AssetLines.Num(), MapLines.Num(), *File);
	return Failures();
}
