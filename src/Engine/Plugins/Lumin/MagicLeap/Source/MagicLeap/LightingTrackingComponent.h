#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "LightingTrackingComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULightingTrackingComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit ULightingTrackingComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
