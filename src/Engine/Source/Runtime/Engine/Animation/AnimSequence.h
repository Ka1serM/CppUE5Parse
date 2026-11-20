#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompressionTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimEnums.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimSequenceBase.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationDecompression.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationPoseData.h"

#include "AnimSequence.generated.h"

namespace ue5parse::runtime {

class UAnimBoneCompressionSettings;
class UAnimCurveCompressionSettings;
class UVariableFrameStrippingSettings;

UENUM()
enum EAdditiveBasePoseType : int32 { ABPT_None, ABPT_RefPose, ABPT_AnimScaled, ABPT_AnimFrame, ABPT_LocalAnimFrame };
UENUM()
enum class EAnimInterpolationType : uint8 { Linear, Step };
UENUM()
enum class EStripAnimDataOnDedicatedServerSettings : uint8 { UseProjectSetting, StripAnimDataOnDedicatedServer, DoNotStripAnimDataOnDedicatedServer };

USTRUCT()
struct FCurveTrack {
    GENERATED_BODY()
    UPROPERTY()
    FName CurveName;
    UPROPERTY()
    TArray<float> CurveWeights;
};

UCLASS()
class UAnimSequence : public UAnimSequenceBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 NumFrames = 0;
    UPROPERTY()
    TObjectPtr<UAnimBoneCompressionSettings> BoneCompressionSettings = nullptr;
    UPROPERTY()
    TObjectPtr<UAnimCurveCompressionSettings> CurveCompressionSettings = nullptr;
    UPROPERTY()
    TObjectPtr<UVariableFrameStrippingSettings> VariableFrameStrippingSettings = nullptr;
    UPROPERTY()
    TEnumAsByte<EAdditiveAnimationType> AdditiveAnimType = AAT_None;
    UPROPERTY()
    TEnumAsByte<EAdditiveBasePoseType> RefPoseType = ABPT_RefPose;
    UPROPERTY()
    int32 RefFrameIndex = 0;
    UPROPERTY()
    TObjectPtr<UAnimSequence> RefPoseSeq = nullptr;
    UPROPERTY()
    FName RetargetSource;
    UPROPERTY()
    TArray<FTransform> RetargetSourceAssetReferencePose;
    UPROPERTY()
    EAnimInterpolationType Interpolation = EAnimInterpolationType::Linear;
    UPROPERTY()
    bool bEnableRootMotion = false;
    UPROPERTY()
    TEnumAsByte<ERootMotionRootLock::Type> RootMotionRootLock = ERootMotionRootLock::RefPose;
    UPROPERTY()
    bool bForceRootLock = false;
    UPROPERTY()
    bool bUseNormalizedRootMotionScale = true;
    UPROPERTY()
    bool bRootMotionSettingsCopiedFromMontage = false;
    UPROPERTY()
    EStripAnimDataOnDedicatedServerSettings StripAnimDataOnDedicatedServer = EStripAnimDataOnDedicatedServerSettings::UseProjectSetting;
    UPROPERTY()
    TArray<FAnimSyncMarker> AuthoredSyncMarkers;
    UPROPERTY()
    FPerPlatformFrameRate PlatformTargetFrameRate;
    UPROPERTY()
    FFrameRate TargetFrameRate;
    UPROPERTY()
    TArray<FTrackToSkeletonMap> TrackToSkeletonMapTable;
    UPROPERTY()
    uint32 bLoopingInterpolation : 1 = 0;
    UPROPERTY()
    TArray<FCurveTrack> CurveData_DEPRECATED;
    UPROPERTY()
    TEnumAsByte<AnimationCompressionFormat> TranslationCompressionFormat = ACF_None;
    UPROPERTY()
    TEnumAsByte<AnimationCompressionFormat> RotationCompressionFormat = ACF_None;
    UPROPERTY()
    TEnumAsByte<AnimationCompressionFormat> ScaleCompressionFormat = ACF_None;
    UPROPERTY()
    TArray<int32> CompressedTrackOffsets;
    UPROPERTY()
    FCompressedOffsetData CompressedScaleOffsets;
    UPROPERTY()
    TEnumAsByte<AnimationKeyFormat> KeyEncodingFormat = AKF_ConstantKeyLerp;
    UPROPERTY()
    int32 EncodingPkgVersion = 0;
    UPROPERTY()
    bool bCalcAdditiveDynamically = false;

    FCompressedAnimSequence CompressedData;

    void Serialize(FArchive& Ar) override;
    bool IsValidAdditive() const override;
    EAdditiveAnimationType GetAdditiveAnimType() const override { return AdditiveAnimType; }
    int32 GetNumberOfSampledKeys() const { return CompressedData.CompressedDataStructure ? CompressedData.CompressedDataStructure->CompressedNumberOfKeys : 0; }
    FName GetRetargetTransformsSourceName() const;
    const TArray<FTransform>& GetRetargetTransforms() const;
    bool HasRootMotion() const override { return bEnableRootMotion; }

    void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const override;
    void GetBonePose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    void GetAdditiveBasePose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    void EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const override;
    float EvaluateCurveData(FName CurveName, float CurrentTime) const;
    FTransform ExtractRootTrackTransform(float Time, const FBoneContainer* RequiredBones) const;
};

}
