#include "ue5parse/runtime/Engine/Particles/ParticleModuleTypeData.h"

#include "ue5parse/runtime/Engine/Engine/StaticMesh.h"
#include "ue5parse/runtime/Engine/Particles/ParticleEmitterInstances.h"

// UE's UParticleModuleTypeDataMesh (ParticleModules.cpp, UE 5.3).

namespace ue5parse::runtime {

void UParticleModuleTypeDataMesh::PostLoad() {
    Super::PostLoad();
    if (Mesh != nullptr)
        Mesh->ConditionalPostLoad();
}

// UE also creates a uniform RollPitchYawRange when none exists; its range is zero, as the empty raw distribution.
FParticleEmitterInstance* UParticleModuleTypeDataMesh::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) {
    FParticleEmitterInstance* Instance = new FParticleMeshEmitterInstance();
    Instance->InitParameters(InEmitterParent, InComponent);
    return Instance;
}

}
