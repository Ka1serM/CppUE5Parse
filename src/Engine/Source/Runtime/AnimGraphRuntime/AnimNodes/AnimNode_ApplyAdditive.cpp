#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_ApplyAdditive.h"

#include "Engine/Source/Runtime/Engine/AnimationRuntime.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimInstanceProxy.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimSequence.h"

namespace ue5parse::runtime {

void FAnimNode_ApplyAdditive::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    Base.Initialize(Context);
    Additive.Initialize(Context);
    AlphaBoolBlend.Reinitialize();
    AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_ApplyAdditive::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    Base.CacheBones(Context);
    Additive.CacheBones(Context);
}

void FAnimNode_ApplyAdditive::Update_AnyThread(const FAnimationUpdateContext& Context) {
    Base.Update(Context);
    GetEvaluateGraphExposedInputs().Execute(Context);
    switch (AlphaInputType) {
        case EAnimAlphaInputType::Float: ActualAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime())); break;
        case EAnimAlphaInputType::Bool: ActualAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime()); break;
        case EAnimAlphaInputType::Curve:
            ActualAlpha = AlphaScaleBiasClamp.ApplyTo(Context.AnimInstanceProxy->GetAnimInstance()->GetCurveValue(AlphaCurveName),
                                                      Context.GetDeltaTime());
            break;
    }
    if (FAnimWeight::IsRelevant(ActualAlpha)) Additive.Update(Context.FractionalWeight(ActualAlpha));
}

void FAnimNode_ApplyAdditive::Evaluate_AnyThread(FPoseContext& Output) {
    if (!FAnimWeight::IsRelevant(ActualAlpha)) {
        Base.Evaluate(Output);
        return;
    }
    FPoseContext AdditiveEvalContext(Output, true);
    Base.Evaluate(Output);
    Additive.Evaluate(AdditiveEvalContext);
    FAnimationPoseData OutAnimationPoseData = Output.GetAnimationPoseData();
    FAnimationRuntime::AccumulateAdditivePose(OutAnimationPoseData, AdditiveEvalContext.GetAnimationPoseData(), ActualAlpha, AAT_LocalSpaceBase);
    Output.Pose.NormalizeRotations();
}

}
