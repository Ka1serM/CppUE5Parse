#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_SkeletalControlBase.generated.h"

namespace ue5parse::runtime {

class USkeleton;

USTRUCT()
struct FAnimNode_SkeletalControlBase : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FComponentSpacePoseLink ComponentPose;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;
    UPROPERTY()
    float ActualAlpha = 0;
    UPROPERTY()
    EAnimAlphaInputType AlphaInputType = EAnimAlphaInputType::Float;
    UPROPERTY()
    bool bAlphaBoolEnabled = true;
    UPROPERTY()
    float Alpha = 1.0f;
    UPROPERTY()
    FInputScaleBias AlphaScaleBias;
    UPROPERTY()
    FInputAlphaBoolBlend AlphaBoolBlend;
    UPROPERTY()
    FName AlphaCurveName;
    UPROPERTY()
    FInputScaleBiasClamp AlphaScaleBiasClamp;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) override;

protected:
    virtual void UpdateInternal(const FAnimationUpdateContext& Context) {}
    virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context) {}
    virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {}
    virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) { return false; }
    virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) {}

private:
    TArray<FBoneTransform> BoneTransforms;
};

}
