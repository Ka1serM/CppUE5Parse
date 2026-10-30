#include "ue5parse/runtime/Engine/Animation/AnimCompositeBase.h"

#include <cmath>

#include "ue5parse/runtime/Core/Math/UnrealMathUtility.h"
#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"

// UE's AnimCompositeBase.cpp (UE 5.3). A cooked segment is always valid (bValid).

namespace ue5parse::runtime {

float FAnimSegment::GetValidPlayRate() const {
    const float SeqPlayRate = AnimReference ? AnimReference->RateScale : 1.0f;
    const float FinalPlayRate = SeqPlayRate * AnimPlayRate;
    return FMath::IsNearlyZero(FinalPlayRate) ? 1.0f : FinalPlayRate;
}

float FAnimSegment::GetLength() const {
    return static_cast<float>(LoopingCount) * (AnimEndTime - AnimStartTime) / std::abs(GetValidPlayRate());
}

UAnimSequenceBase* FAnimSegment::GetAnimationData(float PositionInTrack, float& PositionInAnim) const {
    if (!IsInRange(PositionInTrack) || AnimReference == nullptr) return nullptr;
    const float ValidPlayRate = GetValidPlayRate();
    float Delta = PositionInTrack - StartPos;
    if (LoopingCount > 1) Delta = std::fmod(Delta, (AnimEndTime - AnimStartTime) / std::abs(ValidPlayRate));
    PositionInAnim = ValidPlayRate > 0.0f ? AnimStartTime + Delta * ValidPlayRate : AnimEndTime + Delta * ValidPlayRate;
    return AnimReference;
}

float FAnimSegment::ConvertTrackPosToAnimPos(const float& TrackPosition) const {
    const float PlayRate = GetValidPlayRate();
    const float AnimLength = AnimEndTime - AnimStartTime;
    const float AnimPositionUnWrapped = (TrackPosition - StartPos) * PlayRate;
    // The number of times the animation may loop.
    const float LoopCount = static_cast<float>(FMath::Min(FMath::FloorToInt(FMath::Abs(AnimPositionUnWrapped) / AnimLength), FMath::Max(LoopingCount - 1, 0)));
    const float AnimPoint = PlayRate >= 0.f ? AnimStartTime : AnimEndTime;
    return AnimPoint + (AnimPositionUnWrapped - LoopCount * AnimLength);
}

void FAnimSegment::GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition,
                                                     FAnimNotifyContext& NotifyContext) const {
    const bool bTrackPlayingBackwards = PreviousTrackPosition > CurrentTrackPosition;
    const float SegmentStartPos = StartPos;
    const float SegmentEndPos = StartPos + GetLength();
    const bool bZeroTrackPositionDelta = CurrentTrackPosition == PreviousTrackPosition;
    const bool bOverlaps = bTrackPlayingBackwards ? CurrentTrackPosition < SegmentEndPos && PreviousTrackPosition > SegmentStartPos
                                                  : PreviousTrackPosition < SegmentEndPos && CurrentTrackPosition > SegmentStartPos;
    if (!bOverlaps || AnimReference == nullptr) return;
    const float ValidPlayRate = GetValidPlayRate();
    const float AbsValidPlayRate = FMath::Abs(ValidPlayRate);
    float AnimStartPosition =
        ConvertTrackPosToAnimPos(bTrackPlayingBackwards ? FMath::Min(PreviousTrackPosition, SegmentEndPos) : FMath::Max(PreviousTrackPosition, SegmentStartPos));
    AnimStartPosition = FMath::Clamp(AnimStartPosition, AnimStartTime, AnimEndTime);
    float TrackTimeToGo = FMath::Abs(FMath::Clamp(CurrentTrackPosition, SegmentStartPos, SegmentEndPos) - FMath::Clamp(PreviousTrackPosition, SegmentStartPos, SegmentEndPos));
    // The segment's rate can play the animation backwards on a forward track.
    const bool bAnimPlayingBackwards = bTrackPlayingBackwards ^ (ValidPlayRate < 0.f);
    const float ResetStartPosition = bAnimPlayingBackwards ? AnimEndTime : AnimStartTime;
    const float AnimEndPoint = bAnimPlayingBackwards ? AnimStartTime : AnimEndTime;
    for (int32 IterationsLeft = FMath::Max(LoopingCount, 1); IterationsLeft > 0 && (TrackTimeToGo > 0.f || bZeroTrackPositionDelta); --IterationsLeft) {
        const float TrackTimeToAnimEndPoint = (AnimEndPoint - AnimStartPosition) / AbsValidPlayRate;
        if (FMath::Abs(TrackTimeToGo) < FMath::Abs(TrackTimeToAnimEndPoint)) {
            const float PlayRate = ValidPlayRate * (bTrackPlayingBackwards ? -1.f : 1.f);
            const float AnimEndPosition = TrackTimeToGo * PlayRate + AnimStartPosition;
            AnimReference->GetAnimNotifiesFromDeltaPositions(AnimStartPosition, AnimEndPosition, NotifyContext);
            break;
        }
        AnimReference->GetAnimNotifiesFromDeltaPositions(AnimStartPosition, AnimEndPoint, NotifyContext);
        TrackTimeToGo -= TrackTimeToAnimEndPoint;
        AnimStartPosition = ResetStartPosition;
    }
}

void FAnimTrack::GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition,
                                                   FAnimNotifyContext& NotifyContext) const {
    for (const FAnimSegment& Segment : AnimSegments)
        if (Segment.IsValid()) Segment.GetAnimNotifiesFromTrackPositions(PreviousTrackPosition, CurrentTrackPosition, NotifyContext);
}

float FAnimTrack::GetLength() const {
    float TotalLength = 0;
    for (const FAnimSegment& Segment : AnimSegments) TotalLength = std::max(TotalLength, Segment.GetEndPos());
    return TotalLength;
}

bool FAnimTrack::IsAdditive() const {
    if (AnimSegments.Num() == 0) return false;
    const UAnimSequenceBase* AnimReference = AnimSegments[0].GetAnimReference();
    return AnimReference != nullptr && AnimReference->IsValidAdditive();
}

bool FAnimTrack::IsRotationOffsetAdditive() const {
    return IsAdditive() && AnimSegments[0].GetAnimReference()->GetAdditiveAnimType() == AAT_RotationOffsetMeshSpace;
}

int32 FAnimTrack::GetSegmentIndexAtTime(float InTime) const {
    for (int32 Idx = AnimSegments.Num() - 1; Idx >= 0; Idx--)
        if (AnimSegments[Idx].IsInRange(InTime)) return Idx;
    return INDEX_NONE;
}

const FAnimSegment* FAnimTrack::GetSegmentAtTime(float InTime) const {
    const int32 SegmentIndex = GetSegmentIndexAtTime(InTime);
    return SegmentIndex != INDEX_NONE ? &AnimSegments[SegmentIndex] : nullptr;
}

void FAnimTrack::GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    const float ClampedTime = FMath::Clamp(static_cast<float>(ExtractionContext.CurrentTime), 0.0f, GetLength());
    const FAnimSegment* AnimSegment = GetSegmentAtTime(ClampedTime);
    float PositionInAnim = 0;
    const UAnimSequenceBase* AnimRef = AnimSegment ? AnimSegment->GetAnimationData(ClampedTime, PositionInAnim) : nullptr;
    if (AnimRef == nullptr) {
        OutAnimationPoseData.GetPose().ResetToRefPose();
        return;
    }
    FAnimExtractContext SequenceExtractionContext(ExtractionContext);
    SequenceExtractionContext.CurrentTime = static_cast<double>(PositionInAnim);
    SequenceExtractionContext.DeltaTimeRecord.SetPrevious(static_cast<float>(SequenceExtractionContext.CurrentTime) -
                                                          SequenceExtractionContext.DeltaTimeRecord.Delta);
    SequenceExtractionContext.bExtractRootMotion &= AnimRef->HasRootMotion();
    SequenceExtractionContext.bLooping = AnimSegment->LoopingCount > 1;
    AnimRef->GetAnimationPose(OutAnimationPoseData, SequenceExtractionContext);
}

}
