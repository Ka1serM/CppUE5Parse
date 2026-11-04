#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"

#include "AnimNode_SaveCachedPose.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_SaveCachedPose : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink Pose;
    UPROPERTY()
    FName CachePoseName;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
    void PostGraphUpdate();

private:
    TArray<FAnimationUpdateContext> CachedUpdateContexts;
    FCompactPose CachedPose;
    FBlendedCurve CachedCurve;
    float GlobalWeight = 0;
    FGraphTraversalCounter InitializationCounter;
    FGraphTraversalCounter CachedBonesCounter;
    FGraphTraversalCounter EvaluationCounter;
};

}
