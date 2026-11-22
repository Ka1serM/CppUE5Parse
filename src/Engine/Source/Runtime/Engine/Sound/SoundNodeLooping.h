#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeLooping.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeLooping : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 LoopCount = 1;
    UPROPERTY()
    uint32 bLoopIndefinitely : 1 = 1;
};

}
