#include "ue5parse/runtime/Engine/Animation/AnimNode_SaveCachedPose.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

// A node that several links reach initializes once per graph initialization. (UE 5.3 also tests an update counter
// that it never advances, so that test never passes.)
void FAnimNode_SaveCachedPose::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    if (InitializationCounter.IsSynchronized_Counter(Context.AnimInstanceProxy->GetInitializationCounter())) return;
    InitializationCounter.SynchronizeWith(Context.AnimInstanceProxy->GetInitializationCounter());
    Pose.Initialize(Context);
}

void FAnimNode_SaveCachedPose::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    if (CachedBonesCounter.IsSynchronized_Counter(Context.AnimInstanceProxy->GetCachedBonesCounter())) return;
    CachedBonesCounter.SynchronizeWith(Context.AnimInstanceProxy->GetCachedBonesCounter());
    EvaluationCounter.Reset();
    Pose.CacheBones(Context);
}

void FAnimNode_SaveCachedPose::Update_AnyThread(const FAnimationUpdateContext& Context) { CachedUpdateContexts.Add(Context); }

void FAnimNode_SaveCachedPose::Evaluate_AnyThread(FPoseContext& Output) {
    if (!EvaluationCounter.IsSynchronized_Counter(Output.AnimInstanceProxy->GetEvaluationCounter())) {
        EvaluationCounter.SynchronizeWith(Output.AnimInstanceProxy->GetEvaluationCounter());
        FPoseContext CachingContext(Output);
        Pose.Evaluate(CachingContext);
        CachedPose.MoveBonesFrom(CachingContext.Pose);
        CachedCurve.CopyFrom(CachingContext.Curve);
    }
    Output.Pose.CopyBonesFrom(CachedPose);
    Output.Curve.CopyFrom(CachedCurve);
}

void FAnimNode_SaveCachedPose::PostGraphUpdate() {
    GlobalWeight = 0;
    if (CachedUpdateContexts.Num() == 0) return;
    int32 MaxWeightIdx = 0;
    GlobalWeight = CachedUpdateContexts[0].GetFinalBlendWeight();
    for (int32 Index = 1; Index < CachedUpdateContexts.Num(); ++Index) {
        if (CachedUpdateContexts[Index].GetFinalBlendWeight() <= GlobalWeight) continue;
        GlobalWeight = CachedUpdateContexts[Index].GetFinalBlendWeight();
        MaxWeightIdx = Index;
    }
    Pose.Update(CachedUpdateContexts[MaxWeightIdx]);
    CachedUpdateContexts.Empty();
}

}
