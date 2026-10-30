#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_BoneDrivenController.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;

enum class EDrivenBoneModificationMode : uint8 { AddToInput, ReplaceComponent, AddToRefPose };

enum class EDrivenDestinationMode : uint8 { Bone, MorphTarget, MaterialParameter };

// Drives components of a bone, or a curve, from one component of another bone's change from its reference pose,
// mapped through a curve or a range (UE's BoneControllers/AnimNode_BoneDrivenController.h).
USTRUCT()
struct FAnimNode_BoneDrivenController : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference SourceBone;
    UPROPERTY()
    UCurveFloat* DrivingCurve = nullptr;
    UPROPERTY()
    float Multiplier = 1.0f;
    UPROPERTY()
    double RangeMin = -1.0;
    UPROPERTY()
    double RangeMax = 1.0;
    UPROPERTY()
    double RemappedMin = 0.0;
    UPROPERTY()
    double RemappedMax = 1.0;
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FBoneReference TargetBone;
    UPROPERTY()
    EDrivenDestinationMode DestinationMode = EDrivenDestinationMode::Bone;
    UPROPERTY()
    EDrivenBoneModificationMode ModificationMode = EDrivenBoneModificationMode::AddToInput;
    UPROPERTY()
    TEnumAsByte<EComponentType::Type> SourceComponent = EComponentType::None;
    UPROPERTY()
    bool bUseRange = false;
    UPROPERTY()
    bool bAffectTargetTranslationX = false;
    UPROPERTY()
    bool bAffectTargetTranslationY = false;
    UPROPERTY()
    bool bAffectTargetTranslationZ = false;
    UPROPERTY()
    bool bAffectTargetRotationX = false;
    UPROPERTY()
    bool bAffectTargetRotationY = false;
    UPROPERTY()
    bool bAffectTargetRotationZ = false;
    UPROPERTY()
    bool bAffectTargetScaleX = false;
    UPROPERTY()
    bool bAffectTargetScaleY = false;
    UPROPERTY()
    bool bAffectTargetScaleZ = false;

protected:
    void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context) override;
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

private:
    // The mapped value of the source bone in a pose.
    double ExtractSourceValue(FCSPose<FCompactPose>& Pose) const;
};

}
