#pragma once

#include "Engine/Source/Runtime/Engine/AlphaBlend.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationAsset.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UBlendProfile;

struct FMontageEvaluationState {
    UAnimMontage* Montage = nullptr;
    FAlphaBlend BlendInfo;
    const UBlendProfile* ActiveBlendProfile = nullptr;
    float MontagePosition = 0;
    FDeltaTimeRecord DeltaTimeRecord;
    float BlendStartAlpha = 0;
    bool bIsPlaying = false;
    bool bIsActive = false;
};

}
