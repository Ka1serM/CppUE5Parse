#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "MediaMetadataComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMediaMetadataComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMediaMetadataComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
