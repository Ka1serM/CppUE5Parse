#pragma once

#include "Engine/Source/Runtime/Engine/Engine/SceneCapture.h"

#include "PlanarReflection.generated.h"

namespace ue5parse::runtime {

class UPlanarReflectionComponent;

UCLASS()
class APlanarReflection : public ASceneCapture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPlanarReflectionComponent> PlanarReflectionComponent = nullptr;
    UPROPERTY()
    bool bShowPreviewPlane_DEPRECATED = true;

    explicit APlanarReflection(const FObjectInitializer& ObjectInitializer);
};

}
