#pragma once

#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"

#include "ParticleModuleMaterial.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UCLASS(abstract)
class UParticleModuleMaterialBase : public UParticleModule {
    GENERATED_BODY()
};

UCLASS()
class UParticleModuleMeshMaterial : public UParticleModuleMaterialBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleMeshMaterial(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> MeshMaterials;
};

}
