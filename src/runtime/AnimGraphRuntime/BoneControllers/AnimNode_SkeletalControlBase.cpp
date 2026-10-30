#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_SkeletalControlBase::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    ComponentPose.Initialize(Context);
    AlphaBoolBlend.Reinitialize();
    AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_SkeletalControlBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
    ComponentPose.CacheBones(Context);
}

// The runtime evaluates LOD 0, where every node is enabled.
void FAnimNode_SkeletalControlBase::Update_AnyThread(const FAnimationUpdateContext& Context) {
    ComponentPose.Update(Context);
    GetEvaluateGraphExposedInputs().Execute(Context);
    switch (AlphaInputType) {
        case EAnimAlphaInputType::Float: ActualAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime())); break;
        case EAnimAlphaInputType::Bool: ActualAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime()); break;
        case EAnimAlphaInputType::Curve:
            ActualAlpha = AlphaScaleBiasClamp.ApplyTo(Context.AnimInstanceProxy->GetAnimInstance()->GetCurveValue(AlphaCurveName),
                                                      Context.GetDeltaTime());
            break;
    }
    ActualAlpha = FMath::Clamp(ActualAlpha, 0.0f, 1.0f);
    const FAnimInstanceProxy& Proxy = *Context.AnimInstanceProxy;
    if (FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Proxy.GetSkeleton(), Proxy.GetRequiredBones())) UpdateInternal(Context);
}

void FAnimNode_SkeletalControlBase::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) {
    ComponentPose.EvaluateComponentSpace(Output);
    const FAnimInstanceProxy& Proxy = *Output.AnimInstanceProxy;
    if (!FAnimWeight::IsRelevant(ActualAlpha) || !IsValidToEvaluate(Proxy.GetSkeleton(), Proxy.GetRequiredBones())) return;
    EvaluateComponentSpaceInternal(Output);
    BoneTransforms.Reset();
    EvaluateSkeletalControl_AnyThread(Output, BoneTransforms);
    if (BoneTransforms.Num() > 0) Output.Pose.LocalBlendCSBoneTransforms(BoneTransforms, FMath::Clamp(ActualAlpha, 0.0f, 1.0f));
}

}
