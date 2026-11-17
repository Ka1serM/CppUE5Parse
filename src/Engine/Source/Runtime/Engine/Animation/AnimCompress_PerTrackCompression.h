#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCompress_RemoveLinearKeys.h"

#include "AnimCompress_PerTrackCompression.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCompress_PerTrackCompression : public UAnimCompress_RemoveLinearKeys {
    GENERATED_BODY()
public:
    explicit UAnimCompress_PerTrackCompression(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float MaxZeroingThreshold;
    UPROPERTY()
    float MaxPosDiffBitwise;
    UPROPERTY()
    float MaxAngleDiffBitwise;
    UPROPERTY()
    float MaxScaleDiffBitwise;
    UPROPERTY()
    TArray<TEnumAsByte<AnimationCompressionFormat>> AllowedRotationFormats;
    UPROPERTY()
    TArray<TEnumAsByte<AnimationCompressionFormat>> AllowedTranslationFormats;
    UPROPERTY()
    TArray<TEnumAsByte<AnimationCompressionFormat>> AllowedScaleFormats;
    UPROPERTY()
    bool bResampleAnimation;
    UPROPERTY()
    float ResampledFramerate;
    UPROPERTY()
    int32 MinKeysForResampling;
    UPROPERTY()
    bool bUseAdaptiveError;
    UPROPERTY()
    bool bUseOverrideForEndEffectors;
    UPROPERTY()
    int32 TrackHeightBias;
    UPROPERTY()
    float ParentingDivisor;
    UPROPERTY()
    float ParentingDivisorExponent;
    UPROPERTY()
    bool bUseAdaptiveError2;
    UPROPERTY()
    float RotationErrorSourceRatio;
    UPROPERTY()
    float TranslationErrorSourceRatio;
    UPROPERTY()
    float ScaleErrorSourceRatio;
    UPROPERTY()
    float MaxErrorPerTrackRatio;
    UPROPERTY()
    float PerturbationProbeSize;
};

}
