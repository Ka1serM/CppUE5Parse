#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationAsset.h"

#include "AnimSequenceBase.generated.h"

namespace ue5parse::runtime {

struct FAnimationPoseData;
struct FAnimNotifyContext;
struct FAnimNotifyQueue;

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
    void GetAnimNotifies(const float& StartTime, const float& DeltaTime, FAnimNotifyContext& NotifyContext) const;
    virtual void GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition, FAnimNotifyContext& NotifyContext) const;
    const FRawCurveTracks& GetCurveData() const { return RawCurveData; }

    virtual bool IsValidAdditive() const { return false; }
    virtual EAdditiveAnimationType GetAdditiveAnimType() const { return AAT_None; }
    virtual bool HasRootMotion() const { return false; }
    virtual void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    virtual void EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const;
};

}
