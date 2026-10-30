#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"

#include "AnimNode_SaveCachedPose.generated.h"

namespace ue5parse::runtime {

// A pose that several UseCachedPose nodes read (UE's Animation/AnimNode_SaveCachedPose.h). Its input updates once per
// frame, after the graph, with the largest weight of its users, and evaluates once per evaluation.
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
    // Records the user's context; the input updates in PostGraphUpdate.
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
