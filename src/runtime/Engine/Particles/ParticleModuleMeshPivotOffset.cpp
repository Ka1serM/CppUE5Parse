#include "ue5parse/runtime/Engine/Particles/ParticleModuleMeshPivotOffset.h"

#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"

// VALORANT's UParticleModuleMeshPivotOffset (from the game's binary).

namespace ue5parse::runtime {

void UParticleModuleMeshPivotOffset::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    PARTICLE_ELEMENT(FVector, Pivot);
    Pivot = PivotOffset;
}

}
