#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimSequenceBase.h"

#include "AnimCompositeBase.generated.h"

namespace ue5parse::runtime {

struct FAnimationPoseData;
struct FAnimNotifyContext;

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
    float GetValidPlayRate() const;
    float GetLength() const;
    float GetEndPos() const { return StartPos + GetLength(); }
    bool IsInRange(float CurPos) const { return CurPos >= StartPos && CurPos <= GetEndPos(); }
    UAnimSequenceBase* GetAnimationData(float PositionInTrack, float& PositionInAnim) const;
    float ConvertTrackPosToAnimPos(const float& TrackPosition) const;
    void GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const;
    bool IsValid() const { return AnimEndTime > AnimStartTime && AnimPlayRate != 0.f; }
};

USTRUCT()
struct FAnimTrack {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FAnimSegment> AnimSegments;

    float GetLength() const;
    bool IsAdditive() const;
    bool IsRotationOffsetAdditive() const;
    int32 GetSegmentIndexAtTime(float InTime) const;
    const FAnimSegment* GetSegmentAtTime(float InTime) const;
    void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    void GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const;
};

UCLASS(Abstract)
class UAnimCompositeBase : public UAnimSequenceBase {
    GENERATED_BODY()
};

}
