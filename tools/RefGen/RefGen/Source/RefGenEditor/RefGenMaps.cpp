#include "RefGenGraph.h"
#include "RefTypes.h"
#include "RefGenCatalog.h"
#if UE_AT_LEAST(4, 9) && !UE_AT_LEAST(5, 0)
#include "HierarchicalLOD.h"
#endif
#include "FileHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#if __has_include("StaticMeshComponentLODInfo.h")
#include "StaticMeshComponentLODInfo.h"
#endif
#include "Engine/Level.h"
#if __has_include("Engine/LevelStreamingDynamic.h")
#include "Engine/LevelStreamingDynamic.h"
#else // 4.20: renamed to LevelStreamingDynamic in 4.21
#include "Engine/LevelStreamingKismet.h"
using ULevelStreamingDynamic = ULevelStreamingKismet;
#endif
#include "Engine/LevelScriptBlueprint.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#if UE_AT_LEAST(4, 21) // rect lights are new in 4.21
#include "Engine/RectLight.h"
#endif
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SphereReflectionCapture.h"
#include "Engine/BoxReflectionCapture.h"
#include "Engine/DecalActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/TriggerBox.h"
#include "Engine/Note.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureLightProfile.h"
#include "Animation/SkeletalMeshActor.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Particles/Emitter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/AmbientSound.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#if UE_AT_LEAST(4, 21)
#include "Components/RectLightComponent.h"
#endif
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DecalComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerStart.h"
#if __has_include("Rendering/ColorVertexBuffer.h")
#include "Rendering/ColorVertexBuffer.h"
#endif
#include "StaticMeshResources.h"
#if UE_AT_LEAST(4, 6)
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "LandscapeLayerInfoObject.h"
#else // 4.5: still Engine classes
#include "Landscape/Landscape.h"
#include "Landscape/LandscapeProxy.h"
#include "Landscape/LandscapeLayerInfoObject.h"
#endif
#if UE_AT_LEAST(5, 0)
#include "LandscapeInfo.h"
#include "LandscapeConfigHelper.h"
#endif
#include "RefGenCatalog.h"
#if RG_LEVEL_SEQUENCES
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "MovieScene.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#endif
#if UE_AT_LEAST(4, 20)
#include "Channels/MovieSceneChannelProxy.h"
#elif UE_AT_LEAST(4, 7) // 4.6's WorldFactory.h lacks #pragma once, and UnrealEd.h already includes it
#include "Factories/WorldFactory.h"
#endif
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#if UE_AT_LEAST(4, 26)
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#endif

namespace
{
	using namespace RefGen;

	template <typename T>
	T* Spawn(UWorld* World, const TCHAR* Label, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator, bool bSetLabel = true)
	{
		FActorSpawnParameters Params;
		Params.Name = MakeUniqueObjectName(World->PersistentLevel, T::StaticClass(), FName(Label));
#if !UE_AT_LEAST(4, 8) // 4.7 fails spawns that touch other geometry (PlayerStart on the floor)
		Params.bNoCollisionFail = true;
#endif
		T* Actor = World->SpawnActor<T>(T::StaticClass(), Location, Rotation, Params);
		if (!Actor)
		{
			Fail(FString::Printf(TEXT("spawn %s"), Label));
			return nullptr;
		}
		if (bSetLabel)
			Actor->SetActorLabel(Label);
		return Actor;
	}

	AActor* SpawnClass(UWorld* World, UClass* Class, const TCHAR* Label, const FVector& Location)
	{
		if (!Class)
			return nullptr;
		FActorSpawnParameters Params;
		Params.Name = MakeUniqueObjectName(World->PersistentLevel, Class, FName(Label));
#if !UE_AT_LEAST(4, 8) // 4.7 fails spawns that touch other geometry (PlayerStart on the floor)
		Params.bNoCollisionFail = true;
#endif
		AActor* Actor = World->SpawnActor<AActor>(Class, Location, FRotator::ZeroRotator, Params);
		if (!Actor)
		{
			Fail(FString::Printf(TEXT("spawn %s"), Label));
			return nullptr;
		}
		Actor->SetActorLabel(Label);
		return Actor;
	}

	UStaticMesh* GetStaticMeshOf(UStaticMeshComponent* Component)
	{
#if UE_AT_LEAST(4, 14)
		return Component->GetStaticMesh();
#else
		return Component->StaticMesh;
#endif
	}

	AStaticMeshActor* MeshActor(UWorld* World, const TCHAR* Label, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale = FVector(1))
	{
		AStaticMeshActor* Actor = Spawn<AStaticMeshActor>(World, Label, Location);
		if (!Actor)
			return nullptr;
		// SetStaticMesh refuses static components on old engines ("Calling SetStaticMesh ... but Mobility is Static")
		UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetStaticMesh(Mesh);
		Component->SetMobility(EComponentMobility::Static);
		if (Mesh && GetStaticMeshOf(Component) != Mesh)
			Fail(FString::Printf(TEXT("static mesh of %s"), Label));
		Actor->SetActorScale3D(Scale);
		return Actor;
	}

	UWorld* NewMap(bool bPartitioned)
	{
#if UE_AT_LEAST(5, 0)
		UWorld* World = GEditor->NewMap(bPartitioned);
#elif UE_AT_LEAST(4, 20)
		UWorld* World = GEditor->NewMap();
#else
		// 4.19's UEditorEngine::NewMap dereferences GUnrealEd, which commandlets do not have: do its essential part
		FWorldContext& Context = GEditor->GetEditorWorldContext();
		if (UWorld* Old = Context.World())
		{
			Old->DestroyWorld(true);
			Old->RemoveFromRoot();
			Context.SetCurrentWorld(nullptr);
		}
		UWorldFactory* Factory = NewObject<UWorldFactory>();
		Factory->WorldType = EWorldType::Editor;
		Factory->bInformEngineOfWorld = true;
#if UE_AT_LEAST(4, 7) // 4.6 worlds have no feature level setting
		Factory->FeatureLevel = GEditor->DefaultWorldFeatureLevel;
#endif
		UPackage* Package = CreatePackage(nullptr, nullptr);
		UWorld* World = CastChecked<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Package, TEXT("Untitled"),
			RF_Public | RF_Standalone, nullptr, GWarn));
		Context.SetCurrentWorld(World);
		GWorld = World;
		World->AddToRoot();
		World->UpdateWorldComponents(true, true);
#endif
		if (!World)
			Fail(TEXT("new map"));
		return World;
	}

	void Save(UWorld* World, const TCHAR* Path)
	{
#if UE_AT_LEAST(4, 20)
		if (!UEditorLoadingAndSavingUtils::SaveMap(World, Path))
			Fail(FString::Printf(TEXT("save map %s"), Path));
#else
		// 4.19's SaveMap goes through GUnrealEd (null in commandlets): move the world into place and save it directly
		const FString Name = FPackageName::GetShortName(Path);
		UPackage* Package = World->GetOutermost();
		const ERenameFlags Flags = REN_NonTransactional | REN_DontCreateRedirectors | REN_ForceNoResetLoaders;
		if (Package->GetName() != Path)
			Package->Rename(Path, nullptr, Flags);
		if (World->GetName() != Name)
			World->Rename(*Name, nullptr, Flags);
		FAssetRegistryModule::AssetCreated(World);
		const FString File = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetMapPackageExtension());
		if (!UPackage::SavePackage(Package, World, RF_Standalone, *File, GError, nullptr, false, true, SAVE_NoError))
			Fail(FString::Printf(TEXT("save map %s"), Path));
#endif
	}

	void AddLevelBlueprint(UWorld* World)
	{
		ULevelScriptBlueprint* Script = World->PersistentLevel->GetLevelScriptBlueprint(false);
		if (!Script)
		{
			Fail(TEXT("level blueprint"));
			return;
		}
		FGraph E(Script->UbergraphPages[0]);
		UK2Node_Event* BeginPlay = E.Event(Script, AActor::StaticClass(), TEXT("ReceiveBeginPlay"));
		auto* Print = E.Call(UKismetSystemLibrary::StaticClass(), TEXT("PrintString"));
		E.Default(Print, TEXT("InString"), TEXT("Level BeginPlay"));
		E.Then(BeginPlay, Print);
		FKismetEditorUtilities::CompileBlueprint(Script);
	}

	void AddLandscape(UWorld* World, FAssets& A, bool bPartitioned)
	{
		// label only after Import: on 5.3 SetActorLabel -> PostEditChangeProperty asserts on the not-yet-set LandscapeGuid
		ALandscape* Landscape = Spawn<ALandscape>(World, TEXT("Landscape"), FVector(-6400, -6400, -200), FRotator::ZeroRotator, false);
		if (!Landscape)
			return;
		const int32 Size = 127;
		TArray<uint16> Heights;
		Heights.SetNum(Size * Size);
		for (int32 Y = 0; Y < Size; ++Y)
			for (int32 X = 0; X < Size; ++X)
				Heights[Y * Size + X] = uint16(32768 + 2000 * FMath::Sin(X * 0.1f) * FMath::Cos(Y * 0.1f));
		Landscape->SetActorScale3D(FVector(100, 100, 100));
		// paint layers (weightmaps, made by the catalog with the M_Landscape layer blend): rock on the slopes, grass elsewhere
		if (UMaterialInterface* Material = Recall<UMaterialInterface>(TEXT("M_Landscape")))
			Landscape->LandscapeMaterial = Material;
		TArray<FLandscapeImportLayerInfo> LayerInfos;
		for (const TCHAR* Layer : { TEXT("Grass"), TEXT("Rock") })
			if (ULandscapeLayerInfoObject* Info = Recall<ULandscapeLayerInfoObject>(*FString::Printf(TEXT("LI_%s"), Layer)))
			{
				FLandscapeImportLayerInfo Import((FName(Layer)));
				Import.LayerInfo = Info;
				Import.LayerData.SetNum(Size * Size);
				LayerInfos.Add(Import);
			}
		if (LayerInfos.Num() == 2)
			for (int32 Y = 0; Y < Size; ++Y)
				for (int32 X = 0; X < Size; ++X)
				{
					const uint8 Rock = uint8(FMath::Clamp(FMath::Abs(FMath::Cos(X * 0.1f) * FMath::Cos(Y * 0.1f)) * 320.f - 64.f, 0.f, 255.f));
					LayerInfos[0].LayerData[Y * Size + X] = 255 - Rock;
					LayerInfos[1].LayerData[Y * Size + X] = Rock;
				}
		else
			LayerInfos.Reset();
#if UE_AT_LEAST(4, 23)
		TMap<FGuid, TArray<uint16>> HeightData;
		HeightData.Add(FGuid(), Heights);
		TMap<FGuid, TArray<FLandscapeImportLayerInfo>> Layers;
		Layers.Add(FGuid(), LayerInfos);
#endif
#if UE_AT_LEAST(5, 5)
		Landscape->Import(FGuid::NewGuid(), 0, 0, Size - 1, Size - 1, 1, 63, HeightData, nullptr, Layers, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
#elif UE_AT_LEAST(5, 0)
		Landscape->Import(FGuid::NewGuid(), 0, 0, Size - 1, Size - 1, 1, 63, HeightData, nullptr, Layers, ELandscapeImportAlphamapType::Additive); // layers: const TArray<FLandscapeLayer>* = nullptr
#elif UE_AT_LEAST(4, 23)
		Landscape->Import(FGuid::NewGuid(), 0, 0, Size - 1, Size - 1, 1, 63, HeightData, nullptr, Layers, ELandscapeImportAlphamapType::Additive);
#elif UE_AT_LEAST(4, 11) // 4.22 and older: single heightmap, no edit layers
		Landscape->Import(FGuid::NewGuid(), 0, 0, Size - 1, Size - 1, 1, 63, Heights.GetData(), nullptr, LayerInfos, ELandscapeImportAlphamapType::Additive);
#else // 4.10 and older: vertex counts and component size instead of the min/max quad rectangle
		Landscape->Import(FGuid::NewGuid(), Size, Size, 63, 1, 63, Heights.GetData(), nullptr, LayerInfos);
#endif
		Landscape->SetActorLabel(TEXT("Landscape"));
		UE_LOG(LogRefGen, Display, TEXT("landscape: %d paint layers"), LayerInfos.Num());
#if UE_AT_LEAST(5, 0)
		// World Partition: split the components into one streaming proxy each (what the editor's import does)
		if (bPartitioned)
			if (ULandscapeInfo* Info = Landscape->GetLandscapeInfo())
			{
				TSet<AActor*> Delete;
				if (!FLandscapeConfigHelper::ChangeGridSize(Info, 1, Delete))
					Fail(TEXT("landscape streaming proxies"));
				for (AActor* Actor : Delete)
					if (Actor)
						World->EditorDestroyActor(Actor, false);
			}
#endif
	}

	void Populate(UWorld* World, FAssets& A, bool bFull)
	{
		MeshActor(World, TEXT("Floor"), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")), FVector(0, 0, 0), FVector(60, 60, 1));
		if (AStaticMeshActor* Red = MeshActor(World, TEXT("Mesh_Red"), A.StaticMesh, FVector(0, 0, 50)))
		{
			Red->GetStaticMeshComponent()->SetMaterial(0, A.InstanceRed);
			Red->GetStaticMeshComponent()->SetMaterial(1, A.InstanceChild);
		}
		if (AStaticMeshActor* Painted = MeshActor(World, TEXT("Mesh_VertexPainted"), A.StaticMesh, FVector(400, 0, 50)))
		{
			UStaticMeshComponent* Component = Painted->GetStaticMeshComponent();
			Component->SetLODDataCount(1, 1);
			FStaticMeshComponentLODInfo& Lod = Component->LODData[0];
			const int32 NumVerts = RG_SM_RENDERDATA(A.StaticMesh)->LODResources[0].GetNumVertices();
			TArray<FColor> Colors;
			for (int32 Index = 0; Index < NumVerts; ++Index)
				Colors.Add(FColor(uint8(Index * 13), uint8(Index * 7), uint8(255 - Index), 255));
			Lod.OverrideVertexColors = new FColorVertexBuffer;
			Lod.OverrideVertexColors->InitFromColorArray(Colors);
			BeginInitResource(Lod.OverrideVertexColors);
		}
		if (A.NaniteMesh)
			MeshActor(World, TEXT("Mesh_Nanite"), A.NaniteMesh, FVector(800, 0, 50));
		if (AStaticMeshActor* Masked = MeshActor(World, TEXT("Mesh_Masked"), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")), FVector(-400, 0, 50)))
			Masked->GetStaticMeshComponent()->SetMaterial(0, A.Masked);
		if (AStaticMeshActor* Glass = MeshActor(World, TEXT("Mesh_Translucent"), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")), FVector(-800, 0, 50)))
			Glass->GetStaticMeshComponent()->SetMaterial(0, A.Translucent);
		if (AStaticMeshActor* Glow = MeshActor(World, TEXT("Mesh_Unlit"), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")), FVector(-1200, 0, 50)))
			Glow->GetStaticMeshComponent()->SetMaterial(0, A.Unlit);
		if (AStaticMeshActor* Movable = MeshActor(World, TEXT("Mesh_Movable"), A.StaticMesh, FVector(0, 400, 50)))
		{
			Movable->SetMobility(EComponentMobility::Movable);
			Movable->GetStaticMeshComponent()->SetCastShadow(false);
			Movable->Tags.Add(TEXT("RefTag"));
		}

		if (ASkeletalMeshActor* Skel = Spawn<ASkeletalMeshActor>(World, TEXT("Skeletal_Anim"), FVector(0, -400, 0)))
		{
			USkeletalMeshComponent* Component = Skel->GetSkeletalMeshComponent();
#if UE_AT_LEAST(5, 1)
			Component->SetSkeletalMeshAsset(A.SkeletalMesh);
#else
			Component->SetSkeletalMesh(A.SkeletalMesh);
#endif
			Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			Component->AnimationData.AnimToPlay = A.Bend;
			Component->AnimationData.bSavedLooping = true;
			Component->AnimationData.bSavedPlaying = true;
			Component->SetMorphTarget(TEXT("Bulge"), 0.5f);
		}
		if (A.Actor)
			if (AActor* Actor = SpawnClass(World, A.Actor->GeneratedClass, TEXT("BP_RefActor_Instance"), FVector(1200, 1200, 0)))
			{
				SetPropertyText(Actor, TEXT("Health"), TEXT("12.5"));
				SetPropertyText(Actor, TEXT("Label"), TEXT("\"Placed override\""));
				TInlineComponentArray<UStaticMeshComponent*> Meshes; // no actor constructor before 4.8
				Actor->GetComponents(Meshes);
				for (UStaticMeshComponent* Mesh : Meshes)
					if (Mesh->GetFName() == TEXT("Body"))
					{
						Mesh->SetRelativeLocation(FVector(0, 0, 120));
						Mesh->SetMaterial(0, A.InstanceRed);
					}
			}
		if (A.ActorChild)
			SpawnClass(World, A.ActorChild->GeneratedClass, TEXT("BP_RefActor_Child_Instance"), FVector(-1200, 1200, 0));

		if (ADirectionalLight* Sun = Spawn<ADirectionalLight>(World, TEXT("Sun"), FVector(0, 0, 1000), FRotator(-45, 30, 0)))
		{
			Sun->GetLightComponent()->SetIntensity(8.f);
			Sun->GetLightComponent()->SetLightFunctionMaterial(A.LightFunction);
#if UE_AT_LEAST(4, 26)
			CastChecked<UDirectionalLightComponent>(Sun->GetComponent())->SetAtmosphereSunLight(true);
#endif
		}
		if (APointLight* Point = Spawn<APointLight>(World, TEXT("Point"), FVector(200, 200, 200)))
		{
			Point->PointLightComponent->SetIntensity(8000.f);
			Point->PointLightComponent->SetSourceRadius(20.f);
			Point->PointLightComponent->SetMobility(EComponentMobility::Stationary);
		}
		if (ASpotLight* SpotLight = Spawn<ASpotLight>(World, TEXT("Spot"), FVector(-200, 200, 400), FRotator(-70, 0, 0)))
		{
			SpotLight->SpotLightComponent->SetOuterConeAngle(40.f);
			SpotLight->SpotLightComponent->SetIESTexture(A.LightProfile);
			SpotLight->SpotLightComponent->SetMobility(EComponentMobility::Movable);
		}
#if UE_AT_LEAST(4, 21)
		if (ARectLight* Rect = Spawn<ARectLight>(World, TEXT("Rect"), FVector(0, -800, 300), FRotator(-30, 90, 0)))
		{
			Rect->RectLightComponent->SetSourceWidth(200.f);
			Rect->RectLightComponent->SetSourceHeight(80.f);
#if UE_AT_LEAST(4, 22)
			Rect->RectLightComponent->SetBarnDoorAngle(45.f);
#endif
		}
#endif
		if (ASkyLight* Sky = Spawn<ASkyLight>(World, TEXT("SkyLight"), FVector(0, 0, 600)))
		{
			Sky->GetLightComponent()->SourceType = SLS_SpecifiedCubemap;
			Sky->GetLightComponent()->Cubemap = A.Cube;
			Sky->GetLightComponent()->SetIntensity(1.5f);
		}
#if UE_AT_LEAST(4, 26)
		Spawn<ASkyAtmosphere>(World, TEXT("SkyAtmosphere"), FVector::ZeroVector);
		Spawn<AVolumetricCloud>(World, TEXT("VolumetricCloud"), FVector::ZeroVector);
#endif
		if (AExponentialHeightFog* Fog = Spawn<AExponentialHeightFog>(World, TEXT("HeightFog"), FVector(0, 0, -100)))
		{
			Fog->GetComponent()->SetFogDensity(0.03f);
#if UE_AT_LEAST(4, 16) // volumetric fog is 4.16+
			Fog->GetComponent()->SetVolumetricFog(true);
#endif
		}
		if (APostProcessVolume* Post = Spawn<APostProcessVolume>(World, TEXT("PostProcess"), FVector::ZeroVector))
		{
			Post->bUnbound = true;
			Post->Settings.bOverride_BloomIntensity = true;
			Post->Settings.BloomIntensity = 1.2f;
			Post->Settings.bOverride_AutoExposureBias = true;
			Post->Settings.AutoExposureBias = 0.5f;
			Post->Settings.bOverride_VignetteIntensity = true;
			Post->Settings.VignetteIntensity = 0.6f;
#if UE_AT_LEAST(4, 9)
			Post->Settings.AddBlendable(A.PostProcess, 0.5f);
#else // weighted blendables are 4.9+
			Post->Settings.Blendables.Add(A.PostProcess);
#endif
		}
		Spawn<ASphereReflectionCapture>(World, TEXT("SphereCapture"), FVector(0, 0, 200));
		Spawn<ABoxReflectionCapture>(World, TEXT("BoxCapture"), FVector(600, 600, 200));
		if (ADecalActor* Decal = Spawn<ADecalActor>(World, TEXT("Decal"), FVector(300, -300, 0), FRotator(-90, 0, 0)))
		{
			Decal->GetDecal()->SetDecalMaterial(A.Decal); // ADecalActor's own setter is not exported on 4.8
#if UE_AT_LEAST(4, 11)
			Decal->GetDecal()->DecalSize = FVector(50, 200, 200);
#endif
		}
		if (ACameraActor* Camera = Spawn<ACameraActor>(World, TEXT("Camera"), FVector(-1500, 0, 400), FRotator(-15, 0, 0)))
		{
			RG_SET_FOV(Camera->GetCameraComponent(), 70.f);
			Camera->GetCameraComponent()->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
			Camera->GetCameraComponent()->PostProcessSettings.DepthOfFieldFocalDistance = 1200.f;
		}
		if (ATextRenderActor* Text = Spawn<ATextRenderActor>(World, TEXT("Text"), FVector(0, 0, 400)))
			RG_SET_RENDER_TEXT(Text->GetTextRender(), TEXT("Reference Map"));
		if (AEmitter* Emitter = Spawn<AEmitter>(World, TEXT("Emitter"), FVector(-400, -400, 100)))
			Emitter->SetTemplate(A.Particles);
		if (AAmbientSound* Sound = Spawn<AAmbientSound>(World, TEXT("AmbientSound"), FVector(400, -400, 100)))
		{
			Sound->GetAudioComponent()->SetSound(A.Cue);
			Sound->GetAudioComponent()->AttenuationSettings = A.Attenuation;
		}
		Spawn<ANote>(World, TEXT("Note"), FVector(0, 0, 100));
		if (ATriggerBox* Trigger = Spawn<ATriggerBox>(World, TEXT("Trigger"), FVector(1000, 0, 100)))
			Trigger->SetActorScale3D(FVector(4, 4, 2));
		Spawn<APlayerStart>(World, TEXT("PlayerStart"), FVector(-600, -600, 100));

#if UE_AT_LEAST(4, 7)
		if (AActor* Foliage = Spawn<AActor>(World, TEXT("HISM_Foliage"), FVector(2000, 0, 0)))
#else // AActor itself is abstract in 4.6: hold the instances on a plain editor-only actor
		if (AActor* Foliage = Spawn<ANote>(World, TEXT("HISM_Foliage"), FVector(2000, 0, 0)))
#endif
		{
			auto* Root = NewObject<USceneComponent>(Foliage, TEXT("Root"));
			Foliage->SetRootComponent(Root);
			RG_ADD_INSTANCE_COMPONENT(Foliage, Root);
			Root->RegisterComponent();
			auto* Hism = NewObject<UHierarchicalInstancedStaticMeshComponent>(Foliage, TEXT("Instances"));
#if UE_AT_LEAST(4, 12)
			Hism->SetupAttachment(Root);
#else
			Hism->AttachTo(Root);
#endif
			Hism->SetStaticMesh(A.StaticMesh);
			Hism->SetMaterial(0, A.Masked);
			RG_ADD_INSTANCE_COMPONENT(Foliage, Hism);
			Hism->RegisterComponent();
			for (int32 Index = 0; Index < 64; ++Index)
				Hism->AddInstance(FTransform(FRotator(0, Index * 17.f, 0), FVector((Index % 8) * 200.f, (Index / 8) * 200.f, 0), FVector(0.25f + (Index % 3) * 0.1f)));
		}

		if (ARefNativeActor* Native = Spawn<ARefNativeActor>(World, TEXT("NativeActor"), FVector(0, 1500, 0)))
		{
			for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
				Native->LinkedActors.Add(*It);
			Native->Settings.Count = 3;
			Native->Settings.Label = TEXT("Placed");
		}

#if RG_LEVEL_SEQUENCES
		if (A.Sequence)
			if (ALevelSequenceActor* SequenceActor = Spawn<ALevelSequenceActor>(World, TEXT("Sequence"), FVector::ZeroVector))
				SequenceActor->SetSequence(A.Sequence);
#endif

		AddLandscape(World, A, !bFull);
		PopulateCatalog(World, A, !bFull);
	}

	void BindSequence(ULevelSequence* Sequence, UWorld* World)
	{
#if RG_LEVEL_SEQUENCES
		AStaticMeshActor* Target = nullptr;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
			if (It->GetActorLabel() == TEXT("Mesh_Movable"))
				Target = *It;
		if (!Sequence || !Target)
			return;
		UMovieScene* Scene = Sequence->GetMovieScene();
		const FGuid Binding = Scene->AddPossessable(Target->GetActorLabel(), Target->GetClass());
		Sequence->BindPossessableObject(Binding, *Target, World);
		UMovieScene3DTransformTrack* Track = Scene->AddTrack<UMovieScene3DTransformTrack>(Binding);
		UMovieScene3DTransformSection* Section = Cast<UMovieScene3DTransformSection>(Track->CreateNewSection());
		Track->AddSection(*Section);
#if UE_AT_LEAST(4, 20)
		const FFrameRate Rate = Scene->GetTickResolution();
		Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), (2.0 * Rate).FloorToFrame()));
#if UE_AT_LEAST(5, 0)
		TArrayView<FMovieSceneDoubleChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
#else
		TArrayView<FMovieSceneFloatChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
#endif
		if (Channels.Num() >= 9)
		{
			Channels[0]->AddCubicKey(FFrameNumber(0), 0.0);
			Channels[0]->AddCubicKey((1.0 * Rate).FloorToFrame(), 500.0);
			Channels[0]->AddLinearKey((2.0 * Rate).FloorToFrame(), 0.0);
			Channels[5]->AddConstantKey((1.0 * Rate).FloorToFrame(), 90.0);
			Channels[8]->AddCubicKey((2.0 * Rate).FloorToFrame(), 2.0);
		}
		UMovieSceneFloatTrack* Float = Scene->AddTrack<UMovieSceneFloatTrack>(Binding);
		Float->SetPropertyNameAndPath(TEXT("CustomTimeDilation"), TEXT("CustomTimeDilation"));
		UMovieSceneFloatSection* FloatSection = Cast<UMovieSceneFloatSection>(Float->CreateNewSection());
		Float->AddSection(*FloatSection);
		FloatSection->SetRange(TRange<FFrameNumber>::All());
		FloatSection->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0)->AddLinearKey(FFrameNumber(0), 1.f);
		Scene->SetPlaybackRange(FFrameNumber(0), (2.0 * Rate).FloorToFrame().Value);
#else
		// 4.19 and older: rich curves keyed in seconds instead of frame-number channels
		auto Key = [](FRichCurve& Curve, float Time, float Value, ERichCurveInterpMode Mode)
		{
			Curve.SetKeyInterpMode(Curve.AddKey(Time, Value), Mode);
		};
		Section->SetRange(TRange<float>(0.f, 2.f));
		Key(Section->GetTranslationCurve(EAxis::X), 0.f, 0.f, RCIM_Cubic);
		Key(Section->GetTranslationCurve(EAxis::X), 1.f, 500.f, RCIM_Cubic);
		Key(Section->GetTranslationCurve(EAxis::X), 2.f, 0.f, RCIM_Linear);
		Key(Section->GetRotationCurve(EAxis::Z), 1.f, 90.f, RCIM_Constant);
		Key(Section->GetScaleCurve(EAxis::Z), 2.f, 2.f, RCIM_Cubic);
		UMovieSceneFloatTrack* Float = Scene->AddTrack<UMovieSceneFloatTrack>(Binding);
		Float->SetPropertyNameAndPath(TEXT("CustomTimeDilation"), TEXT("CustomTimeDilation"));
		UMovieSceneFloatSection* FloatSection = Cast<UMovieSceneFloatSection>(Float->CreateNewSection());
		Float->AddSection(*FloatSection);
		FloatSection->SetIsInfinite(true);
		Key(FloatSection->GetFloatCurve(), 0.f, 1.f, RCIM_Linear);
		Scene->SetPlaybackRange(0.f, 2.f);
#endif
		Touch(Sequence);
#endif
	}
}

#include "RefGenCatalogMaps.inl"

void RefGen::MakeSequence(FAssets& A)
{
#if RG_LEVEL_SEQUENCES
	A.Sequence = NewAsset<ULevelSequence>(TEXT("LS_Ref"), TEXT("/Game/Cinematics"));
	A.Sequence->Initialize();
	Touch(A.Sequence);
#endif
}

void RefGen::MakeMaps(FAssets& A)
{
	MakeCatalogMaps(A);
	if (UWorld* Sub = NewMap(false))
	{
		MeshActor(Sub, TEXT("Sub_Mesh"), A.StaticMesh, FVector(3000, 3000, 0));
		if (APointLight* Light = Spawn<APointLight>(Sub, TEXT("Sub_Light"), FVector(3000, 3000, 300)))
			Light->PointLightComponent->SetLightColor(FLinearColor::Green);
		Save(Sub, TEXT("/Game/Maps/L_Ref_Sub"));
	}

	if (UWorld* World = NewMap(false))
	{
		Populate(World, A, true);
		BindSequence(A.Sequence, World);
		AddLevelBlueprint(World);
		ULevelStreamingDynamic* Streaming = NewObject<ULevelStreamingDynamic>(World, NAME_None, RF_Transactional);
		RG_SET_STREAMING_PACKAGE(Streaming, TEXT("/Game/Maps/L_Ref_Sub"));
#if UE_AT_LEAST(4, 20)
		Streaming->SetShouldBeLoaded(true);
		Streaming->SetShouldBeVisible(true);
		RG_SET_STREAMING_COLOR(Streaming, FLinearColor::Green);
		World->AddStreamingLevel(Streaming);
#else
		Streaming->bShouldBeLoaded = true;
		Streaming->bShouldBeVisible = true;
		RG_SET_STREAMING_COLOR(Streaming, FLinearColor::Green);
		World->StreamingLevels.Add(Streaming);
#endif
		if (A.GameMode)
			World->GetWorldSettings()->DefaultGameMode = *A.GameMode->GeneratedClass;
		World->GetWorldSettings()->KillZ = -5000.f;
		Save(World, TEXT("/Game/Maps/L_Ref"));
	}

	// small lit map for bake_win.py: Lightmass lightmaps, shadowmaps and volume samples (MapBuildData from 4.14 on) and UE4
	// hierarchical LOD actors with merged proxy meshes. Kept apart from L_Ref so a bake compiles only a few materials.
	if (UWorld* Lit = NewMap(false))
	{
		MeshActor(Lit, TEXT("Lit_Floor"), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")), FVector(0, 0, 0), FVector(30, 30, 1));
		for (int32 Index = 0; Index < 6; ++Index)
			if (AStaticMeshActor* Mesh = MeshActor(Lit, *FString::Printf(TEXT("Lit_Mesh_%d"), Index), A.StaticMesh, FVector((Index % 3) * 300.f, (Index / 3) * 300.f, 50.f)))
				Mesh->GetStaticMeshComponent()->SetMaterial(0, A.Opaque);
		struct FLight { const TCHAR* Class; const TCHAR* Label; FVector Location; EComponentMobility::Type Mobility; };
		const FLight Lights[] = {
			{ TEXT("DirectionalLight"), TEXT("Lit_Sun"), FVector(0, 0, 1000), EComponentMobility::Stationary },
			{ TEXT("PointLight"), TEXT("Lit_PointStatic"), FVector(300, 0, 300), EComponentMobility::Static },
			{ TEXT("SpotLight"), TEXT("Lit_SpotStationary"), FVector(600, 300, 400), EComponentMobility::Stationary },
			{ TEXT("SkyLight"), TEXT("Lit_Sky"), FVector(0, 0, 800), EComponentMobility::Stationary },
			{ TEXT("SphereReflectionCapture"), TEXT("Lit_Reflection"), FVector(300, 150, 200), EComponentMobility::Static },
		};
		for (const FLight& Light : Lights)
			if (AActor* Actor = SpawnClass(Lit, ClassNamed(Light.Class), Light.Label, Light.Location))
				if (USceneComponent* Root = Actor->GetRootComponent())
					Root->SetMobility(Light.Mobility);
#if UE_AT_LEAST(4, 9) && !UE_AT_LEAST(5, 0)
		// UE4 hierarchical LOD: one cluster of the six meshes (UE5 builds World Partition HLODs for L_RefWP instead). SM_Ref
		// is ~100 m wide, hence the large radius; no material merging, which renders and needs an RHI the commandlet lacks.
		if (!TrySetPropertyText(Lit->GetWorldSettings(), TEXT("bEnableHierarchicalLODSystem"), TEXT("True")))
			Fail(TEXT("hlod: bEnableHierarchicalLODSystem"));
		if (!TrySetPropertyText(Lit->GetWorldSettings(), TEXT("HierarchicalLODSetup"), TEXT("((bSimplifyMesh=False,DesiredBoundRadius=100000.0,DesiredFillingPercentage=10.0,MinNumberOfActorsToBuild=2,MergeSetting=(bMergeMaterials=False)))")))
			Fail(TEXT("hlod: HierarchicalLODSetup"));
#endif
		Save(Lit, TEXT("/Game/Maps/L_RefLit"));
#if UE_AT_LEAST(4, 9) && !UE_AT_LEAST(5, 0)
		// the clusters (LODActors) and their merged proxy meshes, built here on the live editor world: ResavePackages
		// -BuildHLOD finds no actors in a commandlet-loaded map. The proxies go to HLOD packages named after the saved map.
		{
			FHierarchicalLODBuilder Builder(Lit);
			Builder.Build();
#if UE_AT_LEAST(4, 19)
			Builder.BuildMeshesForLODActors(true);
#elif UE_AT_LEAST(4, 12)
			Builder.BuildMeshesForLODActors();
#endif
			int32 LODActors = 0;
			for (AActor* Actor : Lit->PersistentLevel->Actors)
				if (Actor && Actor->GetClass()->GetName() == TEXT("LODActor"))
					++LODActors;
			UE_LOG(LogRefGen, Display, TEXT("hlod: %d LOD actors"), LODActors);
			if (!LODActors)
				Fail(TEXT("hierarchical LOD clusters"));
		}
		Save(Lit, TEXT("/Game/Maps/L_RefLit"));
#endif
	}

#if UE_AT_LEAST(5, 0)
	if (UWorld* World = NewMap(true))
	{
		Populate(World, A, false);
		for (int32 Index = 0; Index < 12; ++Index)
			MeshActor(World, *FString::Printf(TEXT("WP_Mesh_%d"), Index), A.StaticMesh, FVector((Index % 4) * 30000.f, (Index / 4) * 30000.f, 0));
		Save(World, TEXT("/Game/Maps/L_RefWP"));
	}
	// small partitioned map for bake_win.py's World Partition HLOD build (its default instanced and merged HLOD layers)
	if (UWorld* World = NewMap(true))
	{
		for (int32 Index = 0; Index < 16; ++Index)
			if (AStaticMeshActor* Mesh = MeshActor(World, *FString::Printf(TEXT("LitWP_Mesh_%d"), Index), A.StaticMesh, FVector((Index % 4) * 400.f + (Index / 8) * 40000.f, (Index / 4 % 2) * 400.f, 50.f)))
				Mesh->GetStaticMeshComponent()->SetMaterial(0, A.Opaque);
		Save(World, TEXT("/Game/Maps/L_RefLitWP"));
	}
#endif
}
