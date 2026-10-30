#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_RotationOffsetBlendSpaceGraph.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_RotationOffsetBlendSpaceGraph::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_BlendSpaceGraphBase::Initialize_AnyThread(Context);
    BasePose.Initialize(Context);
}

void FAnimNode_RotationOffsetBlendSpaceGraph::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    FAnimNode_BlendSpaceGraphBase::CacheBones_AnyThread(Context);
    BasePose.CacheBones(Context);
}

void FAnimNode_RotationOffsetBlendSpaceGraph::Update_AnyThread(const FAnimationUpdateContext& Context) {
    bIsLODEnabled = true;
    GetEvaluateGraphExposedInputs().Execute(Context);
    switch (AlphaInputType) {
        case EAnimAlphaInputType::Float: ActualAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime())); break;
        case EAnimAlphaInputType::Bool: ActualAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime()); break;
        case EAnimAlphaInputType::Curve:
            ActualAlpha = AlphaScaleBiasClamp.ApplyTo(Context.AnimInstanceProxy->GetAnimInstance()->GetCurveValue(AlphaCurveName), Context.GetDeltaTime());
            break;
    }
    ActualAlpha = FMath::Clamp(ActualAlpha, 0.f, 1.f);
    if (FAnimWeight::IsRelevant(ActualAlpha)) UpdateInternal(Context);
    BasePose.Update(Context);
}

void FAnimNode_RotationOffsetBlendSpaceGraph::Evaluate_AnyThread(FPoseContext& Output) {
    BasePose.Evaluate(Output);
    if (!bIsLODEnabled || !FAnimWeight::IsRelevant(ActualAlpha)) return;
    FPoseContext MeshSpaceRotationAdditivePoseContext(Output);
    FAnimNode_BlendSpaceGraphBase::Evaluate_AnyThread(MeshSpaceRotationAdditivePoseContext);
    FAnimationPoseData BaseAnimationPoseData = Output.GetAnimationPoseData();
    const FAnimationPoseData AdditiveAnimationPoseData = MeshSpaceRotationAdditivePoseContext.GetAnimationPoseData();
    FAnimationRuntime::AccumulateAdditivePose(BaseAnimationPoseData, AdditiveAnimationPoseData, ActualAlpha, AAT_RotationOffsetMeshSpace);
}

}
