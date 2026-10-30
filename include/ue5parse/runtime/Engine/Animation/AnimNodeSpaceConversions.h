#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNodeSpaceConversions.generated.h"

namespace ue5parse::runtime {

// A component space pose as a local pose (UE's Animation/AnimNodeSpaceConversions.h).
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

// A local pose as a component space pose.
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
