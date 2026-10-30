#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_CopyBoneDelta.generated.h"

namespace ue5parse::runtime {

enum class CopyBoneDeltaMode : uint8 { Accumulate, Copy };

// Applies a bone's change from its reference pose, in its parent's space, to another bone: added to it or in place of
// its own (UE's BoneControllers/AnimNode_CopyBoneDelta.h).
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
