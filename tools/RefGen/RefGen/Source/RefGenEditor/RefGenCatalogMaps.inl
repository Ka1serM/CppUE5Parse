// Map side of the catalog: every placeable actor class, every spawnable component class, BSP brushes and volumes,
// foliage and level instances. Included by RefGenMaps.cpp after its helpers (Spawn, NewMap, Save, MeshActor).
#include "RefGenCatalog.h"
#include "Engine/Brush.h"
#include "Engine/BrushBuilder.h"
#include "Engine/Polys.h"
#include "Model.h"
#include "GameFramework/Volume.h"
#include "GameFramework/Info.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/UObjectIterator.h"
#if RG_WITH_FOLIAGE && UE_AT_LEAST(4, 12)
#include "InstancedFoliageActor.h"
#include "InstancedFoliage.h"
#include "FoliageType.h"
#define RG_FOLIAGE 1
#else
#define RG_FOLIAGE 0
#endif
#if UE_AT_LEAST(5, 0)
#include "LevelInstance/LevelInstanceActor.h"
#endif

namespace
{
	const TCHAR* LevelInstanceSource = TEXT("/Game/Catalog/Maps/L_LevelInstanceSource");
	const TCHAR* PackedBlueprint = TEXT("/Game/Catalog/Maps/BP_PackedLevel");

	bool IsEngineClass(UClass* Class)
	{
		const FString Package = Class->GetOutermost()->GetName();
#if UE_AT_LEAST(4, 12)
		if (Class->GetOutermost()->HasAnyPackageFlags(PKG_EditorOnly))
			return false; // editor modules (ComponentVisualizers' Manipulator, ...): not saveable into game content
#endif
#if UE_AT_LEAST(4, 15)
		if (Class->GetDefaultObject()->IsEditorOnly())
			return false;
#endif
		return Package.StartsWith(TEXT("/Script/")) && !Package.Contains(TEXT("Editor")) && Package != TEXT("/Script/UnrealEd")
			&& Package != TEXT("/Script/RefGenEditor");
	}

	bool StartsWithAny(const FString& Name, std::initializer_list<const TCHAR*> Prefixes)
	{
		for (const TCHAR* Prefix : Prefixes)
			if (Name.StartsWith(Prefix))
				return true;
		return false;
	}

	FVector CatalogSlot(int32& Index, float Y)
	{
		const int32 Slot = Index++;
		return FVector(8000.f + (Slot % 16) * 600.f, Y + (Slot / 16) * 600.f, 100.f);
	}

	AActor* CatalogSpawn(UWorld* World, UClass* Class, const FString& Label, const FVector& Location)
	{
		UE_LOG(LogRefGen, Display, TEXT("catalog: spawn %s"), *Class->GetName());
		FActorSpawnParameters Params;
		Params.Name = MakeUniqueObjectName(World->PersistentLevel, Class, FName(*Label));
#if !UE_AT_LEAST(4, 10)
		Params.bNoCollisionFail = true;
#else
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
#endif
		AActor* Actor = World->SpawnActor<AActor>(Class, Location, FRotator::ZeroRotator, Params);
		if (!Actor)
		{
			UE_LOG(LogRefGen, Warning, TEXT("catalog: could not spawn %s"), *Class->GetName());
			return nullptr;
		}
		Actor->SetActorLabel(*Label);
		return Actor;
	}

	void FillComponents(AActor* Actor)
	{
		AssignAssets(Actor);
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			AssignAssets(Component);
			Component->MarkRenderStateDirty();
		}
	}

	// ---------------------------------------------------------------- actors

	// placeable actors that need more setup than a spawn (made elsewhere) or break outside their own editor mode
	bool SkipActor(UClass* Class, bool bPartitioned)
	{
		const FString Name = Class->GetName();
		if (Class->IsChildOf(ABrush::StaticClass()) || Class->IsChildOf(AInfo::StaticClass()))
			return true; // brushes and volumes: AddBrushes; infos: world settings, game state, managers
		if (StartsWithAny(Name, { TEXT("Landscape"), TEXT("WorldPartition"), TEXT("LevelInstance"), TEXT("PackedLevel"), TEXT("InstancedFoliage"),
				TEXT("WorldDataLayers"), TEXT("HLOD"), TEXT("LODActor"), TEXT("Water"), TEXT("PCGWorldActor"), TEXT("PartitionActor"),
				TEXT("ExternalDataLayer"), TEXT("LevelBounds"), TEXT("GameplayDebugger"), TEXT("Abstract"), TEXT("Default__") }))
			return true;
#if !UE_AT_LEAST(4, 5)
		if (Name.StartsWith(TEXT("Functional")))
			return true; // early functional tests crash when spawned outside PIE
#endif
		static const TCHAR* const Names[] = { TEXT("LevelScriptActor"), TEXT("PlayerController"), TEXT("AIController"), TEXT("DebugCameraController"),
			TEXT("SpectatorPawn"), TEXT("Character"), TEXT("WheeledVehicle"), TEXT("WheeledVehiclePawn"), TEXT("NavigationData"), TEXT("RecastNavMesh"),
			TEXT("AbstractNavData"), TEXT("NavigationTestingActor"), TEXT("ReflectionCapture"), TEXT("InstancedPlacementPartitionActor"),
			TEXT("LightWeightInstanceManager"), TEXT("LightWeightInstanceStaticMeshManager"), TEXT("ChaosCacheManager"), TEXT("MovieSceneCameraCutActor"),
			TEXT("MatineeActorCameraAnim"), TEXT("ARActor"), TEXT("ARSkyLight"), TEXT("ARPlaneActor"), TEXT("GeneratedMeshAreaLight"),
			TEXT("VREditorAvatarActor"), TEXT("ServerStatReplicator"), TEXT("ParticleEventManager") };
		for (const TCHAR* Skip : Names)
			if (Name == Skip)
				return true;
		return false;
	}

	struct FGuardedSpawn
	{
		UWorld* World;
		UClass* Class;
		FVector Location;
		static void Run(void* Context)
		{
			FGuardedSpawn* Self = static_cast<FGuardedSpawn*>(Context);
			if (AActor* Actor = CatalogSpawn(Self->World, Self->Class, TEXT("A_") + Self->Class->GetName(), Self->Location))
				FillComponents(Actor);
		}
	};

#if PLATFORM_WINDOWS && !UE_AT_LEAST(4, 5)
	// structured exception guard (no C++ objects with destructors in this frame)
	bool RunGuarded(void (*Fn)(void*), void* Context)
	{
		__try
		{
			Fn(Context);
			return true;
		}
		__except (1)
		{
			return false;
		}
	}
#endif

	void AddActors(UWorld* World, bool bPartitioned)
	{
		TArray<UClass*> Classes;
		for (TObjectIterator<UClass> It; It; ++It)
			if (It->IsChildOf(AActor::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NotPlaceable | CLASS_NewerVersionExists)
				&& IsEngineClass(*It) && !SkipActor(*It, bPartitioned))
				Classes.Add(*It);
		Classes.Sort([](const UClass& L, const UClass& R) { return L.GetName() < R.GetName(); });
		int32 Index = 0;
		for (UClass* Class : Classes)
		{
			FGuardedSpawn Spawn{ World, Class, CatalogSlot(Index, -30000.f) };
#if PLATFORM_WINDOWS && !UE_AT_LEAST(4, 5)
			// 4.3/4.4 have many actors that crash when spawned in a commandlet (Matinee, functional tests, ...)
			if (!RunGuarded(&FGuardedSpawn::Run, &Spawn))
				UE_LOG(LogRefGen, Warning, TEXT("catalog: spawning %s crashed, skipped"), *Class->GetName());
#else
			FGuardedSpawn::Run(&Spawn);
#endif
		}
		UE_LOG(LogRefGen, Display, TEXT("catalog: %d actor classes"), Classes.Num());
	}

	// ---------------------------------------------------------------- components

	bool SkipComponent(UClass* Class)
	{
		static const TCHAR* const Names[] = { TEXT("ChildActorComponent"), TEXT("CharacterMovementComponent"), TEXT("InputComponent"),
			TEXT("TimelineComponent"), TEXT("AIPerceptionComponent"), TEXT("PawnSensingComponent"), TEXT("BrainComponent"), TEXT("BehaviorTreeComponent"),
			TEXT("BlackboardComponent"), TEXT("PathFollowingComponent"), TEXT("CrowdFollowingComponent"), TEXT("GameplayTasksComponent"),
			TEXT("AbilitySystemComponent"), TEXT("ModelComponent"), TEXT("BrushComponent"), TEXT("LandscapeComponent"), TEXT("LandscapeHeightfieldCollisionComponent"),
			TEXT("LandscapeMeshCollisionComponent"), TEXT("FoliageInstancedStaticMeshComponent"), TEXT("StateTreeComponent"), TEXT("StateTreeAIComponent"),
			TEXT("WorldPartitionStreamingSourceComponent"), TEXT("ActorTextureStreamingBuildDataComponent") };
		const FString Name = Class->GetName();
		for (const TCHAR* Skip : Names)
			if (Name == Skip)
				return true;
		return Name.StartsWith(TEXT("Landscape")) || Name.StartsWith(TEXT("Water")) || Name.StartsWith(TEXT("Editor"));
	}

	void SetupComponent(UActorComponent* Component, int32 Seed)
	{
		if (UInstancedStaticMeshComponent* Instanced = Cast<UInstancedStaticMeshComponent>(Component))
		{
			for (int32 Index = 0; Index < 9; ++Index)
				Instanced->AddInstance(FTransform(FRotator(0, Index * 40.f, 0), FVector((Index % 3) * 150.f, (Index / 3) * 150.f, 0), FVector(0.4f)));
		}
		else if (USplineMeshComponent* SplineMesh = Cast<USplineMeshComponent>(Component))
			SplineMesh->SetStartAndEnd(FVector(0, 0, 0), FVector(300, 0, 0), FVector(400, 200, 0), FVector(300, 300, 0));
#if UE_AT_LEAST(4, 13)
		else if (USplineComponent* Spline = Cast<USplineComponent>(Component))
		{
			Spline->AddSplinePoint(FVector(300, 300, 100), ESplineCoordinateSpace::Local, false);
			Spline->AddSplinePoint(FVector(600, 0, 50), ESplineCoordinateSpace::Local, true);
		}
#endif
	}

	void AddComponents(UWorld* World)
	{
		TArray<UClass*> Classes;
		for (TObjectIterator<UClass> It; It; ++It)
			if (It->IsChildOf(UActorComponent::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
				&& It->HasMetaData(TEXT("BlueprintSpawnableComponent")) && IsEngineClass(*It) && !SkipComponent(*It))
				Classes.Add(*It);
		Classes.Sort([](const UClass& L, const UClass& R) { return L.GetName() < R.GetName(); });
		int32 Index = 0;
		for (UClass* Class : Classes)
		{
			const FString Label = TEXT("C_") + Class->GetName();
			AActor* Host = CatalogSpawn(World, AActor::StaticClass(), Label, CatalogSlot(Index, -60000.f));
			if (!Host)
				continue;
			USceneComponent* Root = NewObject<USceneComponent>(Host, TEXT("Root"), RF_Transactional);
			Host->SetRootComponent(Root);
			RG_ADD_INSTANCE_COMPONENT(Host, Root);
			Root->RegisterComponent();
			Root->SetWorldLocation(Host->GetActorLocation());
			UActorComponent* Component = NewObject<UActorComponent>(Host, Class, FName(*Class->GetName()), RF_Transactional);
			AssignAssets(Component);
			if (USceneComponent* Scene = Cast<USceneComponent>(Component))
#if UE_AT_LEAST(4, 12)
				Scene->SetupAttachment(Root);
#else
				Scene->AttachTo(Root);
#endif
			RG_ADD_INSTANCE_COMPONENT(Host, Component);
			Component->RegisterComponent();
			SetupComponent(Component, Index);
		}
		UE_LOG(LogRefGen, Display, TEXT("catalog: %d component classes"), Classes.Num());
	}

	// ---------------------------------------------------------------- brushes and volumes

	// shapes the builder brush as a cube; BRUSH ADD / SUBTRACT / ADDVOLUME then copy it. The polygons are written
	// directly: UBrushBuilder::Build snaps through the level editor's viewports and asserts in some commandlets.
	bool ShapeBuilderBrush(UWorld* World, const FVector& Location, float Size)
	{
		ABrush* Default = World->GetDefaultBrush();
		if (!Default || !Default->Brush || !Default->Brush->Polys)
			return false;
		UModel* Model = Default->Brush;
		Model->Modify();
		Model->Polys->Element.Empty();
		const float H = Size * 0.5f;
		// corner I: bit 0 = +X, bit 1 = +Y, bit 2 = +Z; each face listed so its normal points outwards
		static const int32 Faces[6][4] = { { 0, 4, 6, 2 }, { 1, 3, 7, 5 }, { 0, 1, 5, 4 }, { 2, 6, 7, 3 }, { 0, 2, 3, 1 }, { 4, 5, 7, 6 } };
		for (const auto& Face : Faces)
		{
			FPoly Poly;
			Poly.Init();
			for (int32 Corner : Face)
				Poly.Vertices.Add(decltype(Poly.Base)((Corner & 1) ? H : -H, (Corner & 2) ? H : -H, (Corner & 4) ? H : -H));
			Poly.Base = Poly.Vertices[0];
			if (Poly.Finalize(Default, 1) == 0)
				Model->Polys->Element.Add(Poly);
		}
		Model->BuildBound();
		Default->SetActorLocation(Location);
		return Model->Polys->Element.Num() == 6;
	}

	void AddBrushes(UWorld* World, bool bPartitioned)
	{
		if (!bPartitioned)
		{
			if (ShapeBuilderBrush(World, FVector(-9000, -9000, 200), 600.f))
				GEditor->Exec(World, TEXT("BRUSH ADD"));
			if (ShapeBuilderBrush(World, FVector(-9000, -9000, 400), 300.f))
				GEditor->Exec(World, TEXT("BRUSH SUBTRACT"));
		}
		TArray<UClass*> Classes;
		for (TObjectIterator<UClass> It; It; ++It)
			if (It->IsChildOf(AVolume::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NotPlaceable | CLASS_NewerVersionExists)
				&& IsEngineClass(*It))
			{
				const FString Name = It->GetName();
				if (StartsWithAny(Name, { TEXT("WorldPartition"), TEXT("LevelInstance"), TEXT("Water"), TEXT("Landscape") }))
					continue;
				if (bPartitioned && StartsWithAny(Name, { TEXT("LevelStreaming"), TEXT("HierarchicalLOD"), TEXT("PrecomputedVisibility"), TEXT("MeshMergeCulling") }))
					continue;
				Classes.Add(*It);
			}
		Classes.Sort([](const UClass& L, const UClass& R) { return L.GetName() < R.GetName(); });
		int32 Index = 0;
		for (UClass* Class : Classes)
		{
			UE_LOG(LogRefGen, Display, TEXT("catalog: volume %s"), *Class->GetName());
			if (!ShapeBuilderBrush(World, CatalogSlot(Index, -90000.f), 400.f))
				break;
			const int32 Before = World->PersistentLevel->Actors.Num();
			GEditor->Exec(World, *FString::Printf(TEXT("BRUSH ADDVOLUME CLASS=%s"), *Class->GetPathName()));
			for (int32 Actor = Before; Actor < World->PersistentLevel->Actors.Num(); ++Actor)
				if (AActor* Volume = World->PersistentLevel->Actors[Actor])
					if (Volume->IsA(Class))
					{
						Volume->SetActorLabel(*(TEXT("V_") + Class->GetName()));
						FillComponents(Volume);
					}
		}
		UE_LOG(LogRefGen, Display, TEXT("catalog: %d volume classes"), Classes.Num());
	}

	// ---------------------------------------------------------------- foliage

#if RG_FOLIAGE
	void AddFoliageInstances(AInstancedFoliageActor* Foliage, UFoliageType* Type, const FVector& Origin, int32 Count)
	{
#if UE_AT_LEAST(4, 23)
		FFoliageInfo* Info = nullptr;
#else
		FFoliageMeshInfo* Info = nullptr;
#endif
		const UFoliageType* Used = Foliage->AddFoliageType(Type, &Info);
		if (!Info)
		{
			Fail(TEXT("add foliage type"));
			return;
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FFoliageInstance Instance;
			Instance.Location = Origin + FVector((Index % 10) * 150.f, (Index / 10) * 150.f, 0.f);
			Instance.Rotation = FRotator(0, Index * 37.f, 0);
#if UE_AT_LEAST(5, 0)
			Info->AddInstance(Used, Instance);
#elif UE_AT_LEAST(4, 23)
			Info->AddInstance(Foliage, Used, Instance);
#elif UE_AT_LEAST(4, 16)
			Info->AddInstance(Foliage, Used, Instance, true);
#else
			Info->AddInstance(Foliage, Used, Instance);
#endif
		}
#if UE_AT_LEAST(5, 0)
		Info->Refresh(false, true);
#elif UE_AT_LEAST(4, 23)
		Info->Refresh(Foliage, false, true);
#endif
	}
#endif

	void AddFoliage(UWorld* World, bool bPartitioned)
	{
#if RG_FOLIAGE
		UFoliageType* Type = Recall<UFoliageType>(TEXT("FT_Ref"));
		if (!Type)
			return;
#if UE_AT_LEAST(5, 0)
		if (bPartitioned)
		{
			// World Partition: one partition foliage actor per grid cell the instances fall into (as the foliage mode paints)
			const FVector Origins[] = { FVector(-4000, 4000, 0), FVector(60000, 4000, 0), FVector(4000, 60000, 0) };
			int32 Actors = 0;
			for (const FVector& Origin : Origins)
				if (AInstancedFoliageActor* Foliage = AInstancedFoliageActor::Get(World, true, World->PersistentLevel, Origin))
				{
					AddFoliageInstances(Foliage, Type, Origin, 50);
					++Actors;
				}
			if (!Actors)
				Fail(TEXT("partitioned instanced foliage actor"));
			return;
		}
#else
		if (bPartitioned)
			return;
#endif
		AInstancedFoliageActor* Foliage = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(World->PersistentLevel, true);
		if (!Foliage)
		{
			Fail(TEXT("instanced foliage actor"));
			return;
		}
		AddFoliageInstances(Foliage, Type, FVector(-4000, 4000, 0), 100);
#endif
	}

	// ---------------------------------------------------------------- level instances

	void AddLevelInstances(UWorld* World)
	{
#if UE_AT_LEAST(5, 0)
		const TSoftObjectPtr<UWorld> Source{ FSoftObjectPath(FString(LevelInstanceSource) + TEXT(".") + FPackageName::GetShortName(LevelInstanceSource)) };
		if (UClass* Class = ClassNamed(TEXT("LevelInstance")))
			if (ALevelInstance* Instance = Cast<ALevelInstance>(CatalogSpawn(World, Class, TEXT("LevelInstance"), FVector(-6000, 6000, 0))))
				Instance->SetWorldAsset(Source);
		const FString PackedPath = FString(PackedBlueprint) + TEXT(".") + FPackageName::GetShortName(PackedBlueprint);
		if (UBlueprint* Packed = LoadObject<UBlueprint>(nullptr, *PackedPath, nullptr, LOAD_NoWarn))
			if (Packed->GeneratedClass)
				CatalogSpawn(World, Packed->GeneratedClass, TEXT("PackedLevelActor"), FVector(-6000, 8000, 0));
#endif
	}
}

void RefGen::PopulateCatalog(UWorld* World, FAssets& A, bool bPartitioned)
{
	AddActors(World, bPartitioned);
	AddComponents(World);
	AddBrushes(World, bPartitioned);
	AddFoliage(World, bPartitioned);
	AddLevelInstances(World);
	// a lit, stationary-free corner for lightmaps
	if (APointLight* Light = Spawn<APointLight>(World, TEXT("StaticLight"), FVector(-9000, -9000, 900)))
	{
		Light->PointLightComponent->SetMobility(EComponentMobility::Static);
		Light->PointLightComponent->Intensity = 20000.f;
	}
}

void RefGen::MakeCatalogMaps(FAssets& A)
{
#if UE_AT_LEAST(5, 0)
	// the level that LevelInstance and PackedLevelActor reference
	if (UWorld* Source = NewMap(false))
	{
		MeshActor(Source, TEXT("LI_MeshA"), A.StaticMesh, FVector(0, 0, 0));
		MeshActor(Source, TEXT("LI_MeshB"), A.StaticMesh, FVector(300, 0, 0));
		MeshActor(Source, TEXT("LI_MeshC"), A.StaticMesh, FVector(0, 300, 0));
		Save(Source, LevelInstanceSource);
		const FString SourcePath = FString(LevelInstanceSource) + TEXT(".") + FPackageName::GetShortName(LevelInstanceSource);
		// FPackedLevelActorUtils / FPackedLevelActorBuilder need Slate (not in commandlets): a PackedLevelActor
		// blueprint that references the level; the editor packs its instances when the blueprint is next opened
		if (UClass* PackedClass = ClassNamed(TEXT("PackedLevelActor")))
		{
#if UE_AT_LEAST(4, 26)
			UPackage* Package = CreatePackage(PackedBlueprint);
#else
			UPackage* Package = CreatePackage(nullptr, PackedBlueprint);
#endif
			UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(PackedClass, Package, FName(*FPackageName::GetShortName(PackedBlueprint)),
				BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (Blueprint && Blueprint->GeneratedClass)
			{
				TrySetPropertyText(Blueprint->GeneratedClass->GetDefaultObject(), TEXT("WorldAsset"), *SourcePath);
				FKismetEditorUtilities::CompileBlueprint(Blueprint);
				FAssetRegistryModule::AssetCreated(Blueprint);
				Package->MarkPackageDirty();
			}
			else
				Fail(TEXT("packed level actor blueprint"));
		}
	}
#endif
}
