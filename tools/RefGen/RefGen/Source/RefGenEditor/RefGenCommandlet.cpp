#include "RefGenCommandlet.h"
#ifdef _WIN32
// _exit runs DLL detach (and engine static teardown) on Windows; TerminateProcess skips it like Linux _exit does.
extern "C" __declspec(dllimport) void* __stdcall GetCurrentProcess();
extern "C" __declspec(dllimport) int __stdcall TerminateProcess(void* Process, unsigned int ExitCode);
static void RefGenHardExit(int Code) { TerminateProcess(GetCurrentProcess(), (unsigned int)Code); }
#else
#include <unistd.h>
static void RefGenHardExit(int Code) { _exit(Code); }
#endif
#include "HAL/FileManager.h"
#include "RefGenCommon.h"
#include "RefGenCatalog.h"
#include "FileHelpers.h"
#include "Misc/Paths.h"
#include "ShaderCompiler.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY(LogRefGen);

URefGenCommandlet::URefGenCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 URefGenCommandlet::Main(const FString& Params)
{
	FString ManifestFile;
	if (FParse::Value(*Params, TEXT("Manifest="), ManifestFile))
	{
		const int32 Failed = RefGen::WriteManifest(ManifestFile);
		UE_LOG(LogRefGen, Display, TEXT("RefGen manifest finished with %d failures"), Failed);
		GLog->Flush();
		fflush(stdout);
		RefGenHardExit(Failed == 0 ? 0 : 2);
		return Failed == 0 ? 0 : 2;
	}

	FString SourceDir;
	if (!FParse::Value(*Params, TEXT("Source="), SourceDir))
	{
		UE_LOG(LogRefGen, Error, TEXT("missing -Source=<dir with fbx/wav/ies>"));
		return 1;
	}

	{
		TArray<FString> Existing;
		IFileManager::Get().FindFilesRecursive(Existing, *FPaths::ProjectContentDir(), TEXT("*.u*"), true, false);
		if (Existing.Num() > 0)
		{
			UE_LOG(LogRefGen, Error, TEXT("Content folder is not empty (%d packages). Delete %s before generating."), Existing.Num(), *FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
			return 1;
		}
	}

	RefGen::FAssets A;
	RefGen::MakeTextures(A);
	RefGen::MakeMaterials(A);
	RefGen::MakeMeshes(A, SourceDir);
	RefGen::MakeAnimation(A, SourceDir);
	RefGen::MakeData(A);
	RefGen::MakeAudio(A, SourceDir);
	RefGen::MakeParticles(A);
	RefGen::MakeAnimBlueprint(A);
	RefGen::MakeBlueprints(A);
	RefGen::MakeSequence(A);
	RefGen::MakeCatalog(A, SourceDir);
	RefGen::MakeMaps(A);

	if (GShaderCompilingManager)
		GShaderCompilingManager->FinishAllCompilation();
#if UE_AT_LEAST(4, 20)
	// SaveDirtyPackages saves every dirty package, also the engine's ones the generation touched (materials recompiled
	// for new usage flags etc.): only the project's may be written
	for (TObjectIterator<UPackage> It; It; ++It)
		if (It->IsDirty() && !It->GetName().StartsWith(TEXT("/Game/")))
			It->SetDirtyFlag(false);
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
#else
	// maps are saved by MakeMaps already. 4.18's FEditorFileUtils::SaveDirtyPackages crashes without an editor UI, and
	// 4.19's also saves dirty engine packages (45 EditorMaterials etc.): save the dirty /Game packages directly
	{
		TArray<UPackage*> Dirty;
		for (TObjectIterator<UPackage> It; It; ++It)
			if (It->IsDirty() && It->GetName().StartsWith(TEXT("/Game/")) && !It->ContainsMap())
				Dirty.Add(*It);
		for (UPackage* Package : Dirty)
		{
			UObject* Asset = nullptr;
			TArray<UObject*> Inner; // GetObjectsWithOuter: ForEachObjectWithOuter is 4.9+
			GetObjectsWithOuter(Package, Inner, false);
			for (UObject* O : Inner)
				if (!Asset && O->IsAsset())
					Asset = O;
			if (!Asset)
				continue;
			const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			if (!UPackage::SavePackage(Package, Asset, RF_Standalone, *File, GError, nullptr, false, true, SAVE_NoError))
				UE_LOG(LogRefGen, Error, TEXT("failed to save %s"), *Package->GetName());
		}
		UE_LOG(LogRefGen, Display, TEXT("saved %d packages"), Dirty.Num());
	}
#endif

	UE_LOG(LogRefGen, Display, TEXT("RefGen finished with %d failures"), RefGen::Failures());
	const int32 Result = RefGen::Failures() == 0 ? 0 : 2;
	if (!FParse::Param(*Params, TEXT("CleanExit")))
	{
		// Everything is saved; skip engine shutdown, which crashes in some engine builds.
		GLog->Flush();
		fflush(stdout);
		RefGenHardExit(Result);
	}
	return Result;
}
