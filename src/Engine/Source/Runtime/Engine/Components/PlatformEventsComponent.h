#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "PlatformEventsComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPlatformEventsComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UPlatformEventsComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
