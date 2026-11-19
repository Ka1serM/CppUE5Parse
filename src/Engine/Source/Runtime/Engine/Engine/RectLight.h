#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Light.h"

#include "RectLight.generated.h"

namespace ue5parse::runtime {

class URectLightComponent;

UCLASS()
class ARectLight : public ALight {
    GENERATED_BODY()
public:
    explicit ARectLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<URectLightComponent> RectLightComponent = nullptr;
};

}
