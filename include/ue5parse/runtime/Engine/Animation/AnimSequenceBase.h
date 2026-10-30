#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

#include "AnimSequenceBase.generated.h"

namespace ue5parse::runtime {

struct FAnimationPoseData;
struct FAnimNotifyContext;
struct FAnimNotifyQueue;

// UAnimSequenceBase (UE's Animation/AnimSequenceBase.h): a timed animation with notifies and curves.
UCLASS(Abstract)
class UAnimSequenceBase : public UAnimationAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FAnimNotifyEvent> Notifies;
    UPROPERTY()
    float SequenceLength = 0;
    UPROPERTY()
    FRawCurveTracks RawCurveData;
    UPROPERTY()
    float RateScale = 1;
    UPROPERTY()
    bool bLoop = false;

    float GetPlayLength() const override { return SequenceLength; }
    void TickAssetPlayer(FAnimTickRecord& Instance, FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;
    bool IsNotifyAvailable() const { return Notifies.Num() != 0 && GetPlayLength() > 0.f; }
    // The notifies from StartTime over DeltaTime, around the end when the record loops.
    void GetAnimNotifies(const float& StartTime, const float& DeltaTime, FAnimNotifyContext& NotifyContext) const;
    // The notifies between two positions, without looping; a state is active over its whole range.
    virtual void GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition, FAnimNotifyContext& NotifyContext) const;
    const TArray<FAnimNotifyEvent>& GetNotifies() const { return Notifies; }
    const FRawCurveTracks& GetCurveData() const { return RawCurveData; }

    virtual bool IsValidAdditive() const { return false; }
    virtual EAdditiveAnimationType GetAdditiveAnimType() const { return AAT_None; }
    virtual bool HasRootMotion() const { return false; }
    // The pose and curves at a time. Only a sequence has a pose: another class reports anim.asset_unsupported and gives
    // the reference pose.
    virtual void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    // The authored float curves at a time into OutCurve.
    virtual void EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const;
};

}
