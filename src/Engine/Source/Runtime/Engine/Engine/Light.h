#pragma once

#include "Engine/Source/Runtime/Engine/Components/LightComponent.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Light.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class ALight : public AActor {
    GENERATED_BODY()
public:
    explicit ALight(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<ULightComponent> LightComponent = nullptr;
    UPROPERTY()
    uint8 bEnabled : 1 = false;

};

}
