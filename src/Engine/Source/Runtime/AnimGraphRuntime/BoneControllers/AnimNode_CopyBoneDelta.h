#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_CopyBoneDelta.generated.h"

namespace ue5parse::runtime {

enum class CopyBoneDeltaMode : uint8 { Accumulate, Copy };

USTRUCT()
struct FAnimNode_CopyBoneDelta : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference SourceBone;
    UPROPERTY()
    FBoneReference TargetBone;
    UPROPERTY()
    bool bCopyTranslation = false;
    UPROPERTY()
    bool bCopyRotation = false;
    UPROPERTY()
    bool bCopyScale = false;
    UPROPERTY()
    CopyBoneDeltaMode CopyMode = CopyBoneDeltaMode::Accumulate;
    UPROPERTY()
    float TranslationMultiplier = 1.0f;
    UPROPERTY()
    float RotationMultiplier = 1.0f;
    UPROPERTY()
    float ScaleMultiplier = 1.0f;

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};

}
