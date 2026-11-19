#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Light.h"

#include "SpotLight.generated.h"

namespace ue5parse::runtime {

class USpotLightComponent;

UCLASS()
class ASpotLight : public ALight {
    GENERATED_BODY()
public:
    explicit ASpotLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USpotLightComponent> SpotLightComponent = nullptr;
};

}
