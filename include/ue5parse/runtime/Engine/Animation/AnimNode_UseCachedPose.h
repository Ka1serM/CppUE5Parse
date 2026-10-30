#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_UseCachedPose.generated.h"

namespace ue5parse::runtime {

// A user of a SaveCachedPose node (UE's Animation/AnimNode_UseCachedPose.h).
USTRUCT()
struct FAnimNode_UseCachedPose : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink LinkToCachingNode;
    UPROPERTY()
    FName CachePoseName;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override { LinkToCachingNode.Initialize(Context); }
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override { LinkToCachingNode.CacheBones(Context); }
    void Update_AnyThread(const FAnimationUpdateContext& Context) override { LinkToCachingNode.Update(Context); }
    void Evaluate_AnyThread(FPoseContext& Output) override { LinkToCachingNode.Evaluate(Output); }
};

}
