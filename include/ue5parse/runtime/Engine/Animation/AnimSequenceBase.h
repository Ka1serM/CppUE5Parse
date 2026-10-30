#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

#include "AnimSequenceBase.generated.h"

namespace ue5parse::runtime {

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

    float GetPlayLength() const { return SequenceLength; }
    const TArray<FAnimNotifyEvent>& GetNotifies() const { return Notifies; }
    const FRawCurveTracks& GetCurveData() const { return RawCurveData; }
};

}
