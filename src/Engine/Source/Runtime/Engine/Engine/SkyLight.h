#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "SkyLight.generated.h"

namespace ue5parse::runtime {

class USkyLightComponent;

UCLASS()
class ASkyLight : public AInfo {
    GENERATED_BODY()
public:
    explicit ASkyLight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USkyLightComponent> LightComponent = nullptr;
    UPROPERTY()
    uint8 bEnabled : 1 = false;
};

}
