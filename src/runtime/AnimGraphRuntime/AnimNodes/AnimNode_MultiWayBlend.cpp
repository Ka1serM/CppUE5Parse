#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_MultiWayBlend.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"

namespace ue5parse::runtime {

float FAnimNode_MultiWayBlend::GetTotalAlpha() const {
    float TotalAlpha = 0.f;
    for (const float Alpha : DesiredAlphas) TotalAlpha += Alpha;
    return TotalAlpha;
}

void FAnimNode_MultiWayBlend::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    if (Poses.Num() != DesiredAlphas.Num()) {
        DesiredAlphas.SetNum(Poses.Num());
        for (float& Alpha : DesiredAlphas) Alpha = 0.f;
    }
    UpdateCachedAlphas();
    for (FPoseLink& Pose : Poses) Pose.Initialize(Context);
}

void FAnimNode_MultiWayBlend::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    for (FPoseLink& Pose : Poses) Pose.CacheBones(Context);
}

void FAnimNode_MultiWayBlend::UpdateCachedAlphas() {
    const float TotalAlpha = GetTotalAlpha();
    CachedAlphas.SetNum(DesiredAlphas.Num());
    for (float& Alpha : CachedAlphas) Alpha = 0.f;
    if (AlphaScaleBias.ApplyTo(TotalAlpha) <= ZERO_ANIMWEIGHT_THRESH) return;
    for (int32 AlphaIndex = 0; AlphaIndex < DesiredAlphas.Num(); ++AlphaIndex)
        CachedAlphas[AlphaIndex] = AlphaScaleBias.ApplyTo(bNormalizeAlpha ? DesiredAlphas[AlphaIndex] / TotalAlpha : DesiredAlphas[AlphaIndex]);
}

void FAnimNode_MultiWayBlend::Update_AnyThread(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    UpdateCachedAlphas();
    for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
        if (CachedAlphas[PoseIndex] > ZERO_ANIMWEIGHT_THRESH) Poses[PoseIndex].Update(Context.FractionalWeight(CachedAlphas[PoseIndex]));
}

void FAnimNode_MultiWayBlend::Evaluate_AnyThread(FPoseContext& Output) {
    TArray<FCompactPose> SourcePoses;
    TArray<FBlendedCurve> SourceCurves;
    TArray<float> SourceWeights;
    for (int32 PoseIndex = 0; PoseIndex < Poses.Num() && PoseIndex < CachedAlphas.Num(); ++PoseIndex) {
        const float CurrentAlpha = CachedAlphas[PoseIndex];
        if (CurrentAlpha <= ZERO_ANIMWEIGHT_THRESH) continue;
        FPoseContext PoseContext(Output);
        Poses[PoseIndex].Evaluate(PoseContext);
        SourcePoses.AddDefaulted();
        SourcePoses.Last().MoveBonesFrom(PoseContext.Pose);
        SourceCurves.Add(std::move(PoseContext.Curve));
        SourceWeights.Add(CurrentAlpha);
    }
    if (SourcePoses.Num() == 0) {
        if (bAdditiveNode) Output.ResetToAdditiveIdentity();
        else Output.ResetToRefPose();
        return;
    }
    TArray<const FCompactPose*> PosePointers;
    TArray<const FBlendedCurve*> CurvePointers;
    for (int32 Index = 0; Index < SourcePoses.Num(); ++Index) {
        PosePointers.Add(&SourcePoses[Index]);
        CurvePointers.Add(&SourceCurves[Index]);
    }
    FAnimationPoseData AnimationPoseData = Output.GetAnimationPoseData();
    FAnimationRuntime::BlendPosesTogetherIndirect(PosePointers, CurvePointers, SourceWeights, AnimationPoseData);
    Output.Pose.NormalizeRotations();
}

}
