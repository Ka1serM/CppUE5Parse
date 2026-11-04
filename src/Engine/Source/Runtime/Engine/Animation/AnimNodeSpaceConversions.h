#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNodeSpaceConversions.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_ConvertComponentToLocalSpace : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FComponentSpacePoseLink ComponentPose;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override { ComponentPose.Initialize(Context); }
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override { ComponentPose.CacheBones(Context); }
    void Update_AnyThread(const FAnimationUpdateContext& Context) override { ComponentPose.Update(Context); }
    void Evaluate_AnyThread(FPoseContext& Output) override;
};

USTRUCT()
struct FAnimNode_ConvertLocalToComponentSpace : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink LocalPose;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override { LocalPose.Initialize(Context); }
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override { LocalPose.CacheBones(Context); }
    void Update_AnyThread(const FAnimationUpdateContext& Context) override { LocalPose.Update(Context); }
    void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) override;
};

}
