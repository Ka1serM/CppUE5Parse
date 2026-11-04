#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompositeBase.h"

#include "AnimComposite.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimComposite : public UAnimCompositeBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FAnimTrack AnimationTrack;

    void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const override;
    void GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition, FAnimNotifyContext& NotifyContext) const override;
    EAdditiveAnimationType GetAdditiveAnimType() const override;
    bool IsValidAdditive() const override { return GetAdditiveAnimType() != AAT_None; }
    bool HasRootMotion() const override;
};

}
