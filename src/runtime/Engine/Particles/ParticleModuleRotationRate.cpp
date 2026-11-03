#include "ue5parse/runtime/Engine/Particles/ParticleModuleRotationRate.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleLODLevel.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleRequired.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"
#include "ue5parse/runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/WorldSettings.h"

// UE's s_ParticleModules.cpp (UE 5.3).

namespace ue5parse::runtime {

UParticleModuleMeshRotationRate::UParticleModuleMeshRotationRate(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleMeshRotationRate::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleMeshRotationRate::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
        if (MeshRotationOffset) {
            FVector StartRate = StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
            FVector3f StartValue;
            StartValue.X = StartRate.X * 360.0f;
            StartValue.Y = StartRate.Y * 360.0f;
            StartValue.Z = StartRate.Z * 360.0f;
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
            PayloadData->RotationRateBase += StartValue;
            PayloadData->RotationRate += StartValue;
        }
    }
}

UParticleModuleMeshRotationRate_Seeded::UParticleModuleMeshRotationRate_Seeded(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleMeshRotationRate_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleMeshRotationRateMultiplyLife::UParticleModuleMeshRotationRateMultiplyLife(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleMeshRotationRateMultiplyLife::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
    if (MeshRotationOffset) {
        SPAWN_INIT;
        {
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
            FVector3f RateScale(LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component));
            PayloadData->RotationRate *= RateScale;
        }
    }
}

void UParticleModuleMeshRotationRateMultiplyLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
    if (MeshRotationOffset) {
        BEGIN_UPDATE_LOOP;
        {
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
            FVector3f RateScale(LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component));
            PayloadData->RotationRate *= RateScale;
        }
        END_UPDATE_LOOP;
    }
}

UParticleModuleMeshRotationRateOverLife::UParticleModuleMeshRotationRateOverLife(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleMeshRotationRateOverLife::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
    if (MeshRotationOffset) {
        SPAWN_INIT;
        {
            FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
            FVector3f RateValue(RotRate.GetValue(Particle.RelativeTime, Owner->Component));
            RateValue.X = RateValue.X * 360.0f;
            RateValue.Y = RateValue.Y * 360.0f;
            RateValue.Z = RateValue.Z * 360.0f;
            if (bScaleRotRate == false)
                PayloadData->RotationRate += RateValue;
            else
                PayloadData->RotationRate *= RateValue;
        }
    }
}

void UParticleModuleMeshRotationRateOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
    if (MeshRotationOffset) {
        FMeshRotationPayloadData* PayloadData;
        FVector RateValue;
        if (bScaleRotRate == false) {
            BEGIN_UPDATE_LOOP;
            {
                PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
                RateValue = RotRate.GetValue(Particle.RelativeTime, Owner->Component);
                RateValue.X = RateValue.X * 360.0f;
                RateValue.Y = RateValue.Y * 360.0f;
                RateValue.Z = RateValue.Z * 360.0f;
                PayloadData->RotationRate += FVector3f(RateValue);
            }
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP;
            {
                PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
                RateValue = RotRate.GetValue(Particle.RelativeTime, Owner->Component);
                RateValue.X = RateValue.X * 360.0f;
                RateValue.Y = RateValue.Y * 360.0f;
                RateValue.Z = RateValue.Z * 360.0f;
                PayloadData->RotationRate *= FVector3f(RateValue);
            }
            END_UPDATE_LOOP;
        }
    }
}

UParticleModuleRotationRate::UParticleModuleRotationRate(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

void UParticleModuleRotationRate::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleRotationRate::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        float StartRotRate = (UE_PI / 180.f) * 360.0f * StartRotationRate.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        Particle.RotationRate += StartRotRate;
        Particle.BaseRotationRate += StartRotRate;
    }
}

UParticleModuleRotationRate_Seeded::UParticleModuleRotationRate_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleRotationRate_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleRotationRateMultiplyLife::UParticleModuleRotationRateMultiplyLife(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleRotationRateMultiplyLife::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    float RateScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
    Particle.RotationRate *= RateScale;
}

void UParticleModuleRotationRateMultiplyLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    BEGIN_UPDATE_LOOP;
    float RateScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
    Particle.RotationRate *= RateScale;
    END_UPDATE_LOOP;
}

}
