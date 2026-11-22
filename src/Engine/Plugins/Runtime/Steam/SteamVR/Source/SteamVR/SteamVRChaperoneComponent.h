#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "SteamVRChaperoneComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USteamVRChaperoneComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit USteamVRChaperoneComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
