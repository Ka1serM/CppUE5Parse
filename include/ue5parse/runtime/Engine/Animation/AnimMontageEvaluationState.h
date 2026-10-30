#pragma once

#include "ue5parse/runtime/Engine/AlphaBlend.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UBlendProfile;

// FMontageEvaluationState: what the graph reads of a playing montage in one frame (UE's
// Animation/AnimMontageEvaluationState.h). The anim instance copies it from its montage instances before the update.
struct FMontageEvaluationState {
    UAnimMontage* Montage = nullptr;
    FAlphaBlend BlendInfo;
    const UBlendProfile* ActiveBlendProfile = nullptr;
    float MontagePosition = 0;
    FDeltaTimeRecord DeltaTimeRecord;
    // The linear alpha that the blend started from.
    float BlendStartAlpha = 0;
    bool bIsPlaying = false;
    // Valid and not blending out.
    bool bIsActive = false;
};

}
