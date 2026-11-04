#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_UseCachedPose.generated.h"

namespace ue5parse::runtime {

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
