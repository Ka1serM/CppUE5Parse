#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleSize.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

UParticleModuleSize::UParticleModuleSize(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = false;
}

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleSize::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector Size = StartSize.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
    Particle.Size += FVector3f(Size);
    AdjustParticleBaseSizeForUVFlipping(Size, Owner->CurrentLODLevel->RequiredModule->UVFlippingMode, *InRandomStream);
    Particle.BaseSize += FVector3f(Size);
}

UParticleModuleSize_Seeded::UParticleModuleSize_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleSize_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

UParticleModuleSizeMultiplyLife::UParticleModuleSizeMultiplyLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    MultiplyX = true;
    MultiplyY = true;
    MultiplyZ = true;
}

void UParticleModuleSizeMultiplyLife::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
    if (MultiplyX)
        Particle.Size.X *= SizeScale.X;
    if (MultiplyY)
        Particle.Size.Y *= SizeScale.Y;
    if (MultiplyZ)
        Particle.Size.Z *= SizeScale.Z;
}

void UParticleModuleSizeMultiplyLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    if ((Owner == nullptr) || (Owner->ActiveParticles <= 0) || (Owner->ParticleData == nullptr) || (Owner->ParticleIndices == nullptr))
        return;
    const FRawDistribution* FastDistribution = LifeMultiplier.GetFastRawDistribution();
    if (MultiplyX && MultiplyY && MultiplyZ) {
        if (FastDistribution) {
            FVector3f SizeScale;
            BEGIN_UPDATE_LOOP;
            FastDistribution->GetValue3None(Particle.RelativeTime, &SizeScale.X);
            Particle.Size.X *= SizeScale.X;
            Particle.Size.Y *= SizeScale.Y;
            Particle.Size.Z *= SizeScale.Z;
            END_UPDATE_LOOP;
        } else {
            BEGIN_UPDATE_LOOP {
                FVector3f SizeScale(LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component));
                Particle.Size.X *= SizeScale.X;
                Particle.Size.Y *= SizeScale.Y;
                Particle.Size.Z *= SizeScale.Z;
            }
            END_UPDATE_LOOP;
        }
    } else if ((MultiplyX && !MultiplyY && !MultiplyZ) || (!MultiplyX && MultiplyY && !MultiplyZ) || (!MultiplyX && !MultiplyY && MultiplyZ)) {
        int32 Index = MultiplyX ? 0 : (MultiplyY ? 1 : 2);
        BEGIN_UPDATE_LOOP {
            FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
            Particle.Size[Index] *= SizeScale[Index];
        }
        END_UPDATE_LOOP;
    } else {
        BEGIN_UPDATE_LOOP {
            FVector3f SizeScale(LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component));
            if (MultiplyX)
                Particle.Size.X *= SizeScale.X;
            if (MultiplyY)
                Particle.Size.Y *= SizeScale.Y;
            if (MultiplyZ)
                Particle.Size.Z *= SizeScale.Z;
        }
        END_UPDATE_LOOP;
    }
}

UParticleModuleSizeScale::UParticleModuleSizeScale(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    EnableX = true;
    EnableY = true;
    EnableZ = true;
}

void UParticleModuleSizeScale::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    FVector3f ScaleFactor(SizeScale.GetValue(Particle.RelativeTime, Owner->Component));
    Particle.Size = GetParticleBaseSize(*ParticleBase) * ScaleFactor;
}

void UParticleModuleSizeScale::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    BEGIN_UPDATE_LOOP;
    FVector3f ScaleFactor(SizeScale.GetValue(Particle.RelativeTime, Owner->Component));
    Particle.Size = GetParticleBaseSize(Particle) * ScaleFactor;
    END_UPDATE_LOOP;
}

UParticleModuleSizeScaleBySpeed::UParticleModuleSizeScaleBySpeed(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bUpdateModule = true;
    MaxScale.X = 1;
    MaxScale.Y = 1;
}

void UParticleModuleSizeScaleBySpeed::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    FVector Scale(SpeedScale.X, SpeedScale.Y, 1.0f);
    FVector ScaleMax(MaxScale.X, MaxScale.Y, 1.0f);
    BEGIN_UPDATE_LOOP;
    FVector Size = Scale * Particle.Velocity.Size();
    Size = Size.ComponentMax(FVector(1.0f));
    Size = Size.ComponentMin(ScaleMax);
    Particle.Size = GetParticleBaseSize(Particle) * FVector3f(Size);
    END_UPDATE_LOOP;
}

}
