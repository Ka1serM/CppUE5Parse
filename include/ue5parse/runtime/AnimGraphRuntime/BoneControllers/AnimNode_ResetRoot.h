#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"

#include "AnimNode_ResetRoot.generated.h"

namespace ue5parse::runtime {

// Puts the root back to its reference pose and keeps the component space transforms of its children (UE's
// BoneControllers/AnimNode_ResetRoot.h).
USTRUCT()
struct FAnimNode_ResetRoot : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    bool NeedsOnInitializeAnimInstance() const override { return true; }
    void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override {}

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override { return true; }
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

private:
    TArray<FCompactPoseBoneIndex> RootChildren;
};

}
