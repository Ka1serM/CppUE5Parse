#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeOscillator.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USoundNodeOscillator : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint32 bModulateVolume : 1 = 0;
    UPROPERTY()
    uint32 bModulatePitch : 1 = 0;
    UPROPERTY()
    float AmplitudeMin = 0;
    UPROPERTY()
    float AmplitudeMax = 0;
    UPROPERTY()
    float FrequencyMin = 0;
    UPROPERTY()
    float FrequencyMax = 0;
    UPROPERTY()
    float OffsetMin = 0;
    UPROPERTY()
    float OffsetMax = 0;
    UPROPERTY()
    float CenterMin = 0;
    UPROPERTY()
    float CenterMax = 0;
};

}
