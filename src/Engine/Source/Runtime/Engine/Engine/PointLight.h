#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Light.h"

#include "PointLight.generated.h"

namespace ue5parse::runtime {

class UPointLightComponent;

UCLASS()
class APointLight : public ALight {
    GENERATED_BODY()
public:
    explicit APointLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UPointLightComponent> PointLightComponent = nullptr;
};

}
