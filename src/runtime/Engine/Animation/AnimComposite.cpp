#include "ue5parse/runtime/Engine/Animation/AnimComposite.h"

#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"

// UE's Animation/AnimComposite.cpp (UE 5.3).

namespace ue5parse::runtime {

void UAnimComposite::GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    AnimationTrack.GetAnimationPose(OutAnimationPoseData, ExtractionContext);
    FBlendedCurve& OutCurve = OutAnimationPoseData.GetCurve();
    FBlendedCurve CompositeCurve;
    EvaluateCurveData(CompositeCurve, static_cast<float>(ExtractionContext.CurrentTime));
    OutCurve.Combine(CompositeCurve);
}

void UAnimComposite::GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition, FAnimNotifyContext& NotifyContext) const {
    Super::GetAnimNotifiesFromDeltaPositions(PreviousPosition, CurrentPosition, NotifyContext);
    AnimationTrack.GetAnimNotifiesFromTrackPositions(PreviousPosition, CurrentPosition, NotifyContext);
}

EAdditiveAnimationType UAnimComposite::GetAdditiveAnimType() const {
    int32 TrackAdditiveType = -1;
    for (const FAnimSegment& AnimSegment : AnimationTrack.AnimSegments) {
        const UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference();
        if (AnimReference == nullptr) continue;
        const int32 AnimAdditiveType = AnimReference->GetAdditiveAnimType();
        if (TrackAdditiveType == -1) TrackAdditiveType = AnimAdditiveType;
        else if (TrackAdditiveType != AnimAdditiveType) return AAT_None;
    }
    return TrackAdditiveType == -1 ? AAT_None : static_cast<EAdditiveAnimationType>(TrackAdditiveType);
}

bool UAnimComposite::HasRootMotion() const {
    for (const FAnimSegment& AnimSegment : AnimationTrack.AnimSegments)
        if (AnimSegment.GetAnimReference() != nullptr && AnimSegment.GetAnimReference()->HasRootMotion()) return true;
    return false;
}

}
