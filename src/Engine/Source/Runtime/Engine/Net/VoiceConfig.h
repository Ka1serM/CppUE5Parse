#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "VoiceConfig.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UVOIPTalker : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UVOIPTalker(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
