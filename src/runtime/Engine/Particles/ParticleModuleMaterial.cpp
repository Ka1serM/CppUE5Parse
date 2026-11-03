#include "ue5parse/runtime/Engine/Particles/ParticleModuleMaterial.h"

// UE's ParticleModules_Material.cpp (UE 5.3).

namespace ue5parse::runtime {

UParticleModuleMeshMaterial::UParticleModuleMeshMaterial(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
}

}
