#include "Engine/Source/Runtime/Engine/Particles/ParticleModulePreciseLifetime.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"

namespace ue5parse::runtime {

UParticleModulePreciseLifetime::UParticleModulePreciseLifetime(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
}

void UParticleModulePreciseLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    Owner->SetPreciseLifetimeOffset(Offset);
    float& DeathTime = *reinterpret_cast<float*>(reinterpret_cast<uint8*>(ParticleBase) + Offset);
    DeathTime = ParticleBase->OneOverMaxLifetime > 0.0f ? SpawnTime + Owner->CurrentDelay + 1.0f / ParticleBase->OneOverMaxLifetime : 0.0f;
}

}
