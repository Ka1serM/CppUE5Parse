#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "DecalComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UCLASS()
class UDecalComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UMaterialInterface> DecalMaterial = nullptr;
    UPROPERTY()
    FVector DecalSize = FVector(128.0, 256.0, 256.0);

    explicit UDecalComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
