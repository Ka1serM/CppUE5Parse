#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_ModifyBone.h"

#include "AnimNode_ModifyBones.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_ModifyBones : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FBoneReference> BonesToModify;
    UPROPERTY()
    TArray<FVector> Translations;
    UPROPERTY()
    TArray<FRotator> Rotations;
    UPROPERTY()
    TArray<FVector> Scales;
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

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};

}
