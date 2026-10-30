#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCompositeBase.h"

#include "AnimComposite.generated.h"

namespace ue5parse::runtime {

// An animation made of segments of other animations on one track (UE's Animation/AnimComposite.h).
UCLASS()
class UAnimComposite : public UAnimCompositeBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FAnimTrack AnimationTrack;

    void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const override;
    void GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition, FAnimNotifyContext& NotifyContext) const override;
    // The segments' additive type when they all share one (UE's FAnimTrack::GetTrackAdditiveType).
    EAdditiveAnimationType GetAdditiveAnimType() const override;
    bool IsValidAdditive() const override { return GetAdditiveAnimType() != AAT_None; }
    bool HasRootMotion() const override;
};

}
