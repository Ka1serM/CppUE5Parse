#include "RefGenCommon.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/TypeData/ParticleModuleTypeDataBase.h"
#include "Particles/TypeData/ParticleModuleTypeDataMesh.h"
#include "Particles/Event/ParticleModuleEventGenerator.h"
#include "Particles/Location/ParticleModuleLocationBoneSocket.h"
#include "Particles/Material/ParticleModuleMeshMaterial.h"
#include "Distributions/DistributionFloatConstant.h"
#include "Distributions/DistributionVectorUniform.h"
#include "Factories/ParticleSystemFactoryNew.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/StaticMesh.h"

namespace
{
	using namespace RefGen;

	UParticleModule* AddModule(UParticleSystem* System, UParticleLODLevel* Lod, const TCHAR* ClassName)
	{
		// 5.9 moved the Cascade module classes from Engine into the Cascade plugin's CascadeRuntime module
		UClass* Class = LoadClass<UParticleModule>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), ClassName), nullptr, LOAD_Quiet | LOAD_NoWarn);
		if (!Class)
			Class = LoadClass<UParticleModule>(nullptr, *FString::Printf(TEXT("/Script/CascadeRuntime.%s"), ClassName), nullptr, LOAD_Quiet | LOAD_NoWarn);
		if (!Class)
		{
			Fail(FString::Printf(TEXT("particle module class %s"), ClassName));
			return nullptr;
		}
		UParticleModule* Module = NewObject<UParticleModule>(System, Class, NAME_None, RF_Transactional);
		Module->LODValidity = 1;
		if (UParticleModuleTypeDataBase* TypeData = Cast<UParticleModuleTypeDataBase>(Module))
			Lod->TypeDataModule = TypeData;
		else
			Lod->Modules.Add(Module);
		return Module;
	}

	UParticleEmitter* AddEmitter(UParticleSystem* System, const TCHAR* Name, UMaterialInterface* Material, float Rate)
	{
		UParticleSpriteEmitter* Emitter = NewObject<UParticleSpriteEmitter>(System, NAME_None, RF_Transactional);
		Emitter->EmitterName = Name;
		Emitter->CreateLODLevel(0);
		Emitter->SetToSensibleDefaults();
		UParticleLODLevel* Lod = Emitter->LODLevels[0];
		Lod->RequiredModule->Material = Material;
		if (UDistributionFloatConstant* Spawn = Cast<UDistributionFloatConstant>(Lod->SpawnModule->Rate.Distribution))
			Spawn->Constant = Rate;
		FParticleBurst Burst;
		Burst.Count = 5;
		Burst.Time = 0.f;
		Lod->SpawnModule->BurstList.Add(Burst);
		System->Emitters.Add(Emitter);
		return Emitter;
	}
}

void RefGen::MakeParticles(FAssets& A)
{
	UParticleSystem* System = Create<UParticleSystem>(TEXT("P_Ref"), TEXT("/Game/Effects"), 
#if UE_AT_LEAST(4, 9)
		NewObject<UParticleSystemFactoryNew>());
#else // not exported on 4.8: create it by class name
		NewObject<UFactory>(GetTransientPackage(), LoadClass<UFactory>(nullptr, TEXT("/Script/UnrealEd.ParticleSystemFactoryNew"))));
#endif
	A.Particles = System;
	if (!System)
		return;

	UParticleEmitter* Sprites = AddEmitter(System, TEXT("Sprites"), A.Particle, 30.f);
	{
		UParticleLODLevel* Lod = Sprites->LODLevels[0];
		Lod->RequiredModule->InterpolationMethod = PSUVIM_Linear;
		Lod->RequiredModule->SubImages_Horizontal = 2;
		Lod->RequiredModule->SubImages_Vertical = 2;
		Lod->RequiredModule->bUseLocalSpace = true;
		for (const TCHAR* Module : { TEXT("ParticleModuleColorOverLife"), TEXT("ParticleModuleColor"), TEXT("ParticleModuleAcceleration"),
			TEXT("ParticleModuleAccelerationOverLifetime"), TEXT("ParticleModuleAttractorPoint"), TEXT("ParticleModuleCameraOffset"),
			TEXT("ParticleModuleCollision"), TEXT("ParticleModuleKillBox"), TEXT("ParticleModuleLight"), TEXT("ParticleModuleLocation"),
			TEXT("ParticleModuleLocationPrimitiveSphere"), TEXT("ParticleModuleOrbit"), TEXT("ParticleModuleOrientationAxisLock"),
			TEXT("ParticleModuleParameterDynamic"), TEXT("ParticleModuleRotation"), TEXT("ParticleModuleRotationRate"),
			TEXT("ParticleModuleSizeMultiplyLife"), TEXT("ParticleModuleSizeScaleBySpeed"), TEXT("ParticleModuleSubUV"),
			TEXT("ParticleModuleSourceMovement"), TEXT("ParticleModuleVelocityOverLifetime") })
			AddModule(System, Lod, Module);
		if (auto* Events = Cast<UParticleModuleEventGenerator>(AddModule(System, Lod, TEXT("ParticleModuleEventGenerator"))))
		{
			FParticleEvent_GenerateInfo& Info = RefGen::AddDefaulted(Events->Events);
			Info.Type = EPET_Death;
			Info.CustomName = TEXT("SpriteDied");
			Info.Frequency = 1;
		}
	}

	UParticleEmitter* Meshes = AddEmitter(System, TEXT("Meshes"), A.Particle, 5.f);
	{
		UParticleLODLevel* Lod = Meshes->LODLevels[0];
		if (auto* TypeData = Cast<UParticleModuleTypeDataMesh>(AddModule(System, Lod, TEXT("ParticleModuleTypeDataMesh"))))
		{
			TypeData->Mesh = A.StaticMesh;
			TypeData->bOverrideMaterial = true;
		}
		AddModule(System, Lod, TEXT("ParticleModuleMeshRotation"));
		if (auto* MeshMaterial = Cast<UParticleModuleMeshMaterial>(AddModule(System, Lod, TEXT("ParticleModuleMeshMaterial"))))
			MeshMaterial->MeshMaterials.Add(A.InstanceRed);
		// by name: UParticleModuleLocationBoneSocket isn't exported before 4.12, so Cast<> (StaticClass) doesn't link on Windows
		UParticleModule* BoneModule = AddModule(System, Lod, TEXT("ParticleModuleLocationBoneSocket"));
		if (auto* Bone = BoneModule && BoneModule->GetClass()->GetName() == TEXT("ParticleModuleLocationBoneSocket") ? static_cast<UParticleModuleLocationBoneSocket*>(BoneModule) : nullptr)
		{
			Bone->SourceType = BONESOCKETSOURCE_Bones;
			FLocationBoneSocketInfo& Info = RefGen::AddDefaulted(Bone->SourceLocations);
			Info.BoneSocketName = TEXT("bone_2");
		}
	}

	UParticleEmitter* Gpu = AddEmitter(System, TEXT("GpuSprites"), A.Particle, 200.f);
	AddModule(System, Gpu->LODLevels[0], TEXT("ParticleModuleTypeDataGpu"));

	UParticleEmitter* Beam = AddEmitter(System, TEXT("Beam"), A.Particle, 2.f);
	AddModule(System, Beam->LODLevels[0], TEXT("ParticleModuleTypeDataBeam2"));

	UParticleEmitter* Ribbon = AddEmitter(System, TEXT("Ribbon"), A.Particle, 10.f);
	AddModule(System, Ribbon->LODLevels[0], TEXT("ParticleModuleTypeDataRibbon"));

	for (UParticleEmitter* Emitter : System->Emitters)
		Emitter->UpdateModuleLists();
	System->UpdateAllModuleLists();
	System->LODDistances = { 0.f };
	System->bUseFixedRelativeBoundingBox = true;
	System->FixedRelativeBoundingBox = FBox(FVector(-500), FVector(500));
	Touch(System);
}
