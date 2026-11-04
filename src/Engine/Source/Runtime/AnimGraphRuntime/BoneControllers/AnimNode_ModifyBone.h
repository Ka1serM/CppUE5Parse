#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Engine/AnimationRuntime.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_ModifyBone.generated.h"

namespace ue5parse::runtime {

enum EBoneModificationMode : uint8 { BMM_Ignore, BMM_Replace, BMM_Additive };

USTRUCT()
struct FAnimNode_ModifyBone : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference BoneToModify;
    UPROPERTY()
    FVector Translation = FVector::ZeroVector;
    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY()
    FVector Scale = FVector(1.0);
    UPROPERTY()
    EBoneModificationMode TranslationMode = BMM_Ignore;
    UPROPERTY()
    EBoneModificationMode RotationMode = BMM_Ignore;
    UPROPERTY()
    EBoneModificationMode ScaleMode = BMM_Ignore;
    UPROPERTY()
    EBoneControlSpace TranslationSpace = BCS_ComponentSpace;
    UPROPERTY()
    EBoneControlSpace RotationSpace = BCS_ComponentSpace;
    UPROPERTY()
    EBoneControlSpace ScaleSpace = BCS_ComponentSpace;

    FTransform ModifyBoneTransform(FComponentSpacePoseContext& Output, FCompactPoseBoneIndex BoneIndex) const;

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override { return BoneToModify.IsValidToEvaluate(RequiredBones); }
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override { BoneToModify.Initialize(RequiredBones); }
};

}
