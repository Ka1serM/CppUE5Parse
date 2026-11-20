#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "SteamVRTrackingRefComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USteamVRTrackingReferences : public UActorComponent {
    GENERATED_BODY()
public:
    explicit USteamVRTrackingReferences(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
