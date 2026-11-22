#pragma once

#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeEnveloper.generated.h"

namespace ue5parse::runtime {

class UDistributionFloatConstantCurve;

UCLASS()
class USoundNodeEnveloper : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    float LoopStart = 0.0f;
    UPROPERTY()
    float LoopEnd = 0.0f;
    UPROPERTY()
    float DurationAfterLoop = 0.0f;
    UPROPERTY()
    int32 LoopCount = 0;
    UPROPERTY()
    uint32 bLoopIndefinitely : 1 = 0;
    UPROPERTY()
    uint32 bLoop : 1 = 0;
    UPROPERTY()
    TObjectPtr<UDistributionFloatConstantCurve> VolumeInterpCurve_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UDistributionFloatConstantCurve> PitchInterpCurve_DEPRECATED = nullptr;
    UPROPERTY()
    FRuntimeFloatCurve VolumeCurve;
    UPROPERTY()
    FRuntimeFloatCurve PitchCurve;
    UPROPERTY()
    float PitchMin = 1.0f;
    UPROPERTY()
    float PitchMax = 1.0f;
    UPROPERTY()
    float VolumeMin = 1.0f;
    UPROPERTY()
    float VolumeMax = 1.0f;
};

}
