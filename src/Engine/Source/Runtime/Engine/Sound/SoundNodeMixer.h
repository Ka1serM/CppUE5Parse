#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeMixer.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeMixer : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<float> InputVolume;
};

}
