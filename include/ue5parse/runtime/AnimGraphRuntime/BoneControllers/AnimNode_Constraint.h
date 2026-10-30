#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/AnimationCore/Constraint.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_Constraint.generated.h"

namespace ue5parse::runtime {

enum class EConstraintOffsetOption : uint8 { None, Offset_RefPose };

// A target bone the modified bone is constrained to (UE's BoneControllers/AnimNode_Constraint.h).
USTRUCT()
struct FConstraint {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference TargetBone;
    UPROPERTY()
    EConstraintOffsetOption OffsetOption = EConstraintOffsetOption::Offset_RefPose;
    UPROPERTY()
    ETransformConstraintType TransformType = ETransformConstraintType::Translation;
    UPROPERTY()
    FFilterOptionPerAxis PerAxis;

    int32 ConstraintDataIndex = INDEX_NONE;

    void Initialize(const FBoneContainer& RequiredBones) { TargetBone.Initialize(RequiredBones); }
    bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const { return TargetBone.IsValidToEvaluate(RequiredBones) && PerAxis.IsValid(); }
};

// Constrains a bone's translation, rotation, scale or whole transform to the weighted target bones.
USTRUCT()
struct FAnimNode_Constraint : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference BoneToModify;
    UPROPERTY()
    TArray<FConstraint> ConstraintSetup;
    UPROPERTY()
    TArray<float> ConstraintWeights;

    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

private:
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

    TArray<FConstraintData> ConstraintData;
};

}
