#include "ue5parse/runtime/Engine/Particles/ParticleModuleLifetime.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystemComponent.h"

// UE's ParticleModules.cpp (UE 5.3); PreciseLifetime from VALORANT 13.08.

namespace ue5parse::runtime {

UParticleModuleLifetime::UParticleModuleLifetime(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLifetime::SpawnEx(
    FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    {
        float MaxLifetime = Lifetime.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        if (Particle.OneOverMaxLifetime > 0.f)
            Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
        else
            Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
        Particle.RelativeTime = Particle.RelativeTime > 1.0f ? Particle.RelativeTime : SpawnTime * Particle.OneOverMaxLifetime;
    }
}

float UParticleModuleLifetime::GetMaxLifetime() {
    float Min, Max;
    Lifetime.GetOutRange(Min, Max);
    return Max;
}

float UParticleModuleLifetime::GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data) {
    return Lifetime.GetValue(InTime, Data);
}

UParticleModuleLifetime_Seeded::UParticleModuleLifetime_Seeded(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleLifetime_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner) {
    if (RandomSeedInfo.bResetSeedOnEmitterLooping) {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

float UParticleModuleLifetime_Seeded::GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data) {
    return Lifetime.GetValue(InTime, Data, &GetRandomStream(Owner));
}

UParticleModulePreciseLifetime::UParticleModulePreciseLifetime(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

void UParticleModulePreciseLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    Owner->SetPreciseLifetimeOffset(Offset);
    float& DeathTime = *reinterpret_cast<float*>(reinterpret_cast<uint8*>(ParticleBase) + Offset);
    DeathTime = ParticleBase->OneOverMaxLifetime > 0.0f ? SpawnTime + Owner->CurrentDelay + 1.0f / ParticleBase->OneOverMaxLifetime : 0.0f;
}

uint32 UParticleModulePreciseLifetime::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return 4;
}

}
