#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimSequenceBase.h"

#include "AnimCompositeBase.generated.h"

namespace ue5parse::runtime {

struct FAnimationPoseData;
struct FAnimNotifyContext;

// FAnimSegment: a range of an animation placed on a track (UE's Animation/AnimCompositeBase.h).
USTRUCT()
struct FAnimSegment {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UAnimSequenceBase> AnimReference = nullptr;
    UPROPERTY()
    float StartPos = 0;
    UPROPERTY()
    float AnimStartTime = 0;
    UPROPERTY()
    float AnimEndTime = 0;
    UPROPERTY()
    float AnimPlayRate = 1;
    UPROPERTY()
    int32 LoopingCount = 1;

    UAnimSequenceBase* GetAnimReference() const { return AnimReference; }
    // The segment's rate times its animation's rate scale; 1 for a rate near 0.
    float GetValidPlayRate() const;
    float GetLength() const;
    float GetEndPos() const { return StartPos + GetLength(); }
    bool IsInRange(float CurPos) const { return CurPos >= StartPos && CurPos <= GetEndPos(); }
    // The animation at a track position and the position in it; null outside the segment.
    UAnimSequenceBase* GetAnimationData(float PositionInTrack, float& PositionInAnim) const;
    // The position in the animation of a track position, with the segment's loops.
    float ConvertTrackPosToAnimPos(const float& TrackPosition) const;
    // The animation's notifies between two track positions, through the segment's loops.
    void GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const;
    bool IsValid() const { return AnimEndTime > AnimStartTime && AnimPlayRate != 0.f; }
};

USTRUCT()
struct FAnimTrack {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FAnimSegment> AnimSegments;

    float GetLength() const;
    // The first segment's animation decides.
    bool IsAdditive() const;
    bool IsRotationOffsetAdditive() const;
    // The last segment that contains the time: a segment's end overlaps the next one's start.
    int32 GetSegmentIndexAtTime(float InTime) const;
    const FAnimSegment* GetSegmentAtTime(float InTime) const;
    // The pose of the segment at the track time, or the reference pose where no segment is.
    void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    void GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const;
};

// UAnimCompositeBase: an asset made of segments of other animations.
UCLASS(Abstract)
class UAnimCompositeBase : public UAnimSequenceBase {
    GENERATED_BODY()
};

}
