#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleMaterial.h"

namespace ue5parse::runtime {

UParticleModuleMeshMaterial::UParticleModuleMeshMaterial(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

}
