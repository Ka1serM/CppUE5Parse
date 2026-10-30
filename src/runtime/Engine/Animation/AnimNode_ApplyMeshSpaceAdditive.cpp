#include "ue5parse/runtime/Engine/Animation/AnimNode_ApplyMeshSpaceAdditive.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_ApplyMeshSpaceAdditive::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    Base.Initialize(Context);
    Additive.Initialize(Context);
    AlphaBoolBlend.Reinitialize();
    AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_ApplyMeshSpaceAdditive::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    Base.CacheBones(Context);
    Additive.CacheBones(Context);
}

// UE applies the scale bias before the clamp here, unlike the other alpha nodes.
void FAnimNode_ApplyMeshSpaceAdditive::Update_AnyThread(const FAnimationUpdateContext& Context) {
    Base.Update(Context);
    GetEvaluateGraphExposedInputs().Execute(Context);
    ActualAlpha = 0.f;
    switch (AlphaInputType) {
        case EAnimAlphaInputType::Float: ActualAlpha = AlphaScaleBiasClamp.ApplyTo(AlphaScaleBias.ApplyTo(Alpha), Context.GetDeltaTime()); break;
        case EAnimAlphaInputType::Bool: ActualAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime()); break;
        case EAnimAlphaInputType::Curve:
            ActualAlpha = AlphaScaleBiasClamp.ApplyTo(Context.AnimInstanceProxy->GetAnimInstance()->GetCurveValue(AlphaCurveName), Context.GetDeltaTime());
            break;
    }
    ActualAlpha = FMath::Clamp(ActualAlpha, 0.0f, 1.0f);
    if (FAnimWeight::IsRelevant(ActualAlpha)) Additive.Update(Context.FractionalWeight(ActualAlpha));
}

void FAnimNode_ApplyMeshSpaceAdditive::Evaluate_AnyThread(FPoseContext& Output) {
    if (!FAnimWeight::IsRelevant(ActualAlpha)) {
        Base.Evaluate(Output);
        return;
    }
    FPoseContext AdditiveEvalContext(Output, true);
    Base.Evaluate(Output);
    Additive.Evaluate(AdditiveEvalContext);
    FAnimationPoseData BaseAnimationPoseData = Output.GetAnimationPoseData();
    const FAnimationPoseData AdditiveAnimationPoseData = AdditiveEvalContext.GetAnimationPoseData();
    FAnimationRuntime::AccumulateAdditivePose(BaseAnimationPoseData, AdditiveAnimationPoseData, ActualAlpha, AAT_RotationOffsetMeshSpace);
}

}
