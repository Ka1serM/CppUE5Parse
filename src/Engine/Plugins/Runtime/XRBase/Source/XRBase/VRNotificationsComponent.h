#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "VRNotificationsComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UVRNotificationsComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UVRNotificationsComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
