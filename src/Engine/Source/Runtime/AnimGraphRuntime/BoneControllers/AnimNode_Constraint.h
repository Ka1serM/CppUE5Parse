#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/AnimationCore/Constraint.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_Constraint.generated.h"

namespace ue5parse::runtime {

enum class EConstraintOffsetOption : uint8 { None, Offset_RefPose };

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
