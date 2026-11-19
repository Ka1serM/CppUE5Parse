#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Light.h"

#include "DirectionalLight.generated.h"

namespace ue5parse::runtime {

class UDirectionalLightComponent;

UCLASS()
class ADirectionalLight : public ALight {
    GENERATED_BODY()
public:
    explicit ADirectionalLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UDirectionalLightComponent> DirectionalLightComponent = nullptr;
};

}
