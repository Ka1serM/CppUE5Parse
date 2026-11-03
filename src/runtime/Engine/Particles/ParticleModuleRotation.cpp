#include "ue5parse/runtime/Engine/Particles/ParticleModuleRotation.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleLODLevel.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleRequired.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/WorldSettings.h"

// UE's s_ParticleModules.cpp (UE 5.3).

namespace ue5parse::runtime {

UParticleModuleMeshRotation::UParticleModuleMeshRotation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bInheritParent = false;
}

void UParticleModuleMeshRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleMeshRotation::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
        if (MeshRotationOffset) {
            FVector Rotation = StartRotation.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
            if (bInheritParent) {
                FRotator Rotator = Owner->Component->GetComponentRotation();
                FVector ParentAffectedRotation = Rotator.Euler();
                Rotation.X += ParentAffectedRotation.X / 360.0f;
                Rotation.Y += ParentAffectedRotation.Y / 360.0f;
                Rotation.Z += ParentAffectedRotation.Z / 360.0f;
            }
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
            PayloadData->InitRotation = FVector3f(Rotation) * 360.0f;
            PayloadData->Rotation += PayloadData->InitRotation;
        }
    }
}

UParticleModuleMeshRotation_Seeded::UParticleModuleMeshRotation_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleMeshRotation_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleRotation::UParticleModuleRotation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

void UParticleModuleRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleRotation::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        Particle.Rotation += (UE_PI / 180.f) * 360.0f * StartRotation.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
    }
}

UParticleModuleRotation_Seeded::UParticleModuleRotation_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleRotation_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleRotationOverLifetime::UParticleModuleRotationOverLifetime(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = false;
    bUpdateModule = true;
    Scale = true;
}

void UParticleModuleRotationOverLifetime::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if (Scale) {
        BEGIN_UPDATE_LOOP;
        {
            float Rotation = RotationOverLife.GetValue(Particle.RelativeTime, Owner->Component);
            Particle.Rotation = (Particle.Rotation * (Rotation * (UE_PI / 180.f) * 360.0f));
        }
        END_UPDATE_LOOP;
    } else {
        BEGIN_UPDATE_LOOP;
        {
            float Rotation = RotationOverLife.GetValue(Particle.RelativeTime, Owner->Component);
            Particle.Rotation = (Particle.Rotation + (Rotation * (UE_PI / 180.f) * 360.0f));
        }
        END_UPDATE_LOOP;
    }
}

}
