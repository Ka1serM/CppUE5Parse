#include "Engine/Source/Runtime/Engine/Animation/AnimCompress_PerTrackCompression.h"

namespace ue5parse::runtime {

UAnimCompress_PerTrackCompression::UAnimCompress_PerTrackCompression(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    MaxPosDiffBitwise = 0.007f;
    MaxAngleDiffBitwise = 0.002f;
    MaxScaleDiffBitwise = 0.0007f;
    MaxZeroingThreshold = 0.0002f;
    ResampledFramerate = 15.0f;
    bResampleAnimation = false;
    MinKeysForResampling = 10;
    bRetarget = false;
    bActuallyFilterLinearKeys = false;
    bUseAdaptiveError = false;
    ParentingDivisor = 1.0f;
    ParentingDivisorExponent = 1.0f;
    TrackHeightBias = 1;
    bUseOverrideForEndEffectors = false;
    bUseAdaptiveError2 = false;
    RotationErrorSourceRatio = 0.8f;
    TranslationErrorSourceRatio = 0.8f;
    ScaleErrorSourceRatio = 0.001f;
    MaxErrorPerTrackRatio = 0.3f;
    PerturbationProbeSize = 0.001f;
    AllowedRotationFormats.Add(ACF_Identity);
    AllowedRotationFormats.Add(ACF_Fixed48NoW);
    AllowedTranslationFormats.Add(ACF_Identity);
    AllowedTranslationFormats.Add(ACF_IntervalFixed32NoW);
    AllowedTranslationFormats.Add(ACF_Fixed48NoW);
    AllowedScaleFormats.Add(ACF_Identity);
    AllowedScaleFormats.Add(ACF_IntervalFixed32NoW);
    AllowedScaleFormats.Add(ACF_Fixed48NoW);
}

}
