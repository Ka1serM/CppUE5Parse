#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeModulator.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeModulator : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    float PitchMin = 0.95f;
    UPROPERTY()
    float PitchMax = 1.05f;
    UPROPERTY()
    float VolumeMin = 0.95f;
    UPROPERTY()
    float VolumeMax = 1.05f;
};

}
