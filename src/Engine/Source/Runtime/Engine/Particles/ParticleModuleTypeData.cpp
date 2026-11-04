#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"

#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"

namespace ue5parse::runtime {

void UParticleModuleTypeDataMesh::PostLoad() {
    Super::PostLoad();
    if (Mesh != nullptr)
        Mesh->ConditionalPostLoad();
}

FParticleEmitterInstance* UParticleModuleTypeDataMesh::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) {
    FParticleEmitterInstance* Instance = new FParticleMeshEmitterInstance();
    Instance->InitParameters(InEmitterParent, InComponent);
    return Instance;
}

}
