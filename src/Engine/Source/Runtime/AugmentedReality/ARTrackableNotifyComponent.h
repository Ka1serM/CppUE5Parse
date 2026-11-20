#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "ARTrackableNotifyComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UARTrackableNotifyComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UARTrackableNotifyComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
