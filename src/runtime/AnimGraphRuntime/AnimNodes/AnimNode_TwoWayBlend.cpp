#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_TwoWayBlend.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_TwoWayBlend::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    A.Initialize(Context);
    B.Initialize(Context);
    bAIsRelevant = false;
    bBIsRelevant = false;
    AlphaBoolBlend.Reinitialize();
    AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_TwoWayBlend::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    A.CacheBones(Context);
    B.CacheBones(Context);
}

void FAnimNode_TwoWayBlend::Update_AnyThread(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    InternalBlendAlpha = 0.0f;
    switch (AlphaInputType) {
        case EAnimAlphaInputType::Float:
            InternalBlendAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime()));
            break;
        case EAnimAlphaInputType::Bool: InternalBlendAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime()); break;
        case EAnimAlphaInputType::Curve:
            InternalBlendAlpha = AlphaScaleBiasClamp.ApplyTo(Context.AnimInstanceProxy->GetAnimInstance()->GetCurveValue(AlphaCurveName),
                                                             Context.GetDeltaTime());
            break;
    }
    InternalBlendAlpha = FMath::Clamp(InternalBlendAlpha, 0.0f, 1.0f);

    const bool bNewAIsRelevant = !FAnimWeight::IsFullWeight(InternalBlendAlpha);
    const bool bNewBIsRelevant = FAnimWeight::IsRelevant(InternalBlendAlpha);
    if (bResetChildOnActivation) {
        const FAnimationInitializeContext ReinitializeContext(Context.AnimInstanceProxy);
        if (bNewAIsRelevant && !bAIsRelevant) A.Initialize(ReinitializeContext);
        if (bNewBIsRelevant && !bBIsRelevant) B.Initialize(ReinitializeContext);
    }
    bAIsRelevant = bNewAIsRelevant;
    bBIsRelevant = bNewBIsRelevant;
    if (!bBIsRelevant) {
        A.Update(Context);
        return;
    }
    if (!bAIsRelevant) {
        B.Update(Context);
        return;
    }
    A.Update(Context.FractionalWeight(1.0f - InternalBlendAlpha));
    B.Update(Context.FractionalWeight(InternalBlendAlpha));
}

void FAnimNode_TwoWayBlend::Evaluate_AnyThread(FPoseContext& Output) {
    if (!bBIsRelevant) {
        A.Evaluate(Output);
        return;
    }
    if (!bAIsRelevant) {
        B.Evaluate(Output);
        return;
    }
    FPoseContext Pose1(Output);
    FPoseContext Pose2(Output);
    A.Evaluate(Pose1);
    B.Evaluate(Pose2);
    FAnimationPoseData BlendedAnimationPoseData = Output.GetAnimationPoseData();
    FAnimationRuntime::BlendTwoPosesTogether(Pose1.GetAnimationPoseData(), Pose2.GetAnimationPoseData(), 1.0f - InternalBlendAlpha,
                                             BlendedAnimationPoseData);
}

}
