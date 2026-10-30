#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_MakeDynamicAdditive.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"

namespace ue5parse::runtime {

void FAnimNode_MakeDynamicAdditive::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    Base.Initialize(Context);
    Additive.Initialize(Context);
}

void FAnimNode_MakeDynamicAdditive::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    Base.CacheBones(Context);
    Additive.CacheBones(Context);
}

void FAnimNode_MakeDynamicAdditive::Update_AnyThread(const FAnimationUpdateContext& Context) {
    Base.Update(Context.FractionalWeight(1.f));
    Additive.Update(Context.FractionalWeight(1.f));
}

// The inputs are full poses; the output is the additive.
void FAnimNode_MakeDynamicAdditive::Evaluate_AnyThread(FPoseContext& Output) {
    FScopedExpectsAdditiveOverride ScopedExpectsAdditiveOverride(Output, false);
    FPoseContext BaseEvalContext(Output);
    Base.Evaluate(BaseEvalContext);
    Additive.Evaluate(Output);
    if (bMeshSpaceAdditive) {
        FAnimationRuntime::ConvertPoseToMeshRotation(Output.Pose);
        FAnimationRuntime::ConvertPoseToMeshRotation(BaseEvalContext.Pose);
    }
    FAnimationRuntime::ConvertPoseToAdditive(Output.Pose, BaseEvalContext.Pose);
    Output.Curve.ConvertToAdditive(BaseEvalContext.Curve);
}

}
