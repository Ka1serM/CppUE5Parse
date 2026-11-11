#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleMeshPivotOffset.h"

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"

namespace ue5parse::runtime {

void UParticleModuleMeshPivotOffset::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    SPAWN_INIT;
    PARTICLE_ELEMENT(FVector, Pivot);
    Pivot = PivotOffset;
}

}
