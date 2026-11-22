#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeConcatenator.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeConcatenator : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<float> InputVolume;
};

}
