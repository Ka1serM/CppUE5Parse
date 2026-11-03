#pragma once

#include "ue5parse/runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleMaterial.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

// UParticleModuleMaterialBase (UE's Particles/Material/ParticleModuleMaterialBase.h).
UCLASS(abstract)
class UParticleModuleMaterialBase : public UParticleModule {
    GENERATED_BODY()
};

// UParticleModuleMeshMaterial: the materials of the sections of the mesh of a mesh emitter (UE's
// Particles/Material/ParticleModuleMeshMaterial.h).
UCLASS()
class UParticleModuleMeshMaterial : public UParticleModuleMaterialBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshMaterial(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> MeshMaterials;
};

}
