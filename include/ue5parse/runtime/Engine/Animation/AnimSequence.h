#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCompressionTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimationDecompression.h"
#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequenceBase.h"

#include "AnimSequence.generated.h"

namespace ue5parse::runtime {

class UAnimBoneCompressionSettings;
class UAnimCurveCompressionSettings;

enum EAdditiveAnimationType : int32 { AAT_None, AAT_LocalSpaceBase, AAT_RotationOffsetMeshSpace };
enum EAdditiveBasePoseType : int32 { ABPT_None, ABPT_RefPose, ABPT_AnimScaled, ABPT_AnimFrame, ABPT_LocalAnimFrame };
enum class EAnimInterpolationType : uint8 { Linear, Step };

// UAnimSequence (UE's Animation/AnimSequence.h): a clip of bone tracks and curves, as compressed by the cook.
UCLASS()
class UAnimSequence : public UAnimSequenceBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    UAnimBoneCompressionSettings* BoneCompressionSettings = nullptr;
    UPROPERTY()
    UAnimCurveCompressionSettings* CurveCompressionSettings = nullptr;
    UPROPERTY()
    TEnumAsByte<EAdditiveAnimationType> AdditiveAnimType = AAT_None;
    UPROPERTY()
    TEnumAsByte<EAdditiveBasePoseType> RefPoseType = ABPT_RefPose;
    UPROPERTY()
    int32 RefFrameIndex = 0;
    UPROPERTY()
    UAnimSequence* RefPoseSeq = nullptr;
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
    // UE's default sampling rate: 30 frames per second.
    UPROPERTY()
    FPerPlatformFrameRate PlatformTargetFrameRate;

    FCompressedAnimSequence CompressedData;

    void Serialize(FArchive& Ar) override;
    // UE's IsValidAdditive: an additive type and a base pose it can use.
    bool IsValidAdditive() const;
    int32 GetNumberOfSampledKeys() const { return CompressedData.CompressedDataStructure ? CompressedData.CompressedDataStructure->CompressedNumberOfKeys : 0; }
    FName GetRetargetTransformsSourceName() const;
    // The pose the clip was authored on: its own copy, else the skeleton's retarget source of the name (UE's
    // GetRetargetTransforms).
    const TArray<FTransform>& GetRetargetTransforms() const;
    bool HasRootMotion() const { return bEnableRootMotion; }

    // The pose and curves at a time (UE's GetAnimationPose): a baked additive gives the additive delta.
    void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    void GetBonePose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
    // The compressed curves at a time into OutCurve.
    void EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const;
    float EvaluateCurveData(FName CurveName, float CurrentTime) const;
    // The root bone's transform at a time.
    FTransform ExtractRootTrackTransform(float Time, const FBoneContainer* RequiredBones) const;
};

}
