#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_SequenceEvaluator.h"

#include "ue5parse/runtime/Engine/Animation/AnimSequenceBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

float FAnimNode_SequenceEvaluatorBase::GetCurrentAssetLength() const {
    const UAnimSequenceBase* CurrentSequence = GetSequence();
    return CurrentSequence ? CurrentSequence->GetPlayLength() : 0.0f;
}

UAnimationAsset* FAnimNode_SequenceEvaluatorBase::GetAnimAsset() const { return GetSequence(); }

void FAnimNode_SequenceEvaluatorBase::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
    GetEvaluateGraphExposedInputs().Execute(Context);
    bReinitialized = true;
}

void FAnimNode_SequenceEvaluatorBase::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    float CurrentExplicitTime = GetExplicitTime();
    UAnimSequenceBase* CurrentSequence = GetSequence();
    if (CurrentSequence == nullptr) {
        bReinitialized = false;
        return;
    }
    CurrentExplicitTime = FMath::Clamp(CurrentExplicitTime, 0.f, CurrentSequence->GetPlayLength());
    const bool bAdvances = !GetTeleportToExplicitTime() || !GetGroupName().isNone() || GetGroupMethod() == EAnimSyncMethod::Graph;
    if (bAdvances && CurrentSequence->GetSkeleton() != nullptr) {
        if (bReinitialized) {
            switch (GetReinitializationBehavior()) {
                case ESequenceEvalReinit::StartPosition: InternalTimeAccumulator = GetStartPosition(); break;
                case ESequenceEvalReinit::ExplicitTime: InternalTimeAccumulator = CurrentExplicitTime; break;
                default: break;
            }
            InternalTimeAccumulator = FMath::Clamp(InternalTimeAccumulator, 0.f, CurrentSequence->GetPlayLength());
        }
        const float TimeJump = GetEffectiveDeltaTime(CurrentExplicitTime, InternalTimeAccumulator);
        if (TimeJump == 0.f) InternalTimeAccumulator = CurrentExplicitTime;
        // The play rate that moves to the input time in this frame.
        const float DeltaTime = Context.GetDeltaTime();
        const float RateScale = CurrentSequence->RateScale;
        const float PlayRate = FMath::IsNearlyZero(DeltaTime) || FMath::IsNearlyZero(RateScale) ? 0.f : TimeJump / (DeltaTime * RateScale);
        CreateTickRecordForNode(Context, CurrentSequence, IsLooping(), PlayRate, true);
    } else {
        InternalTimeAccumulator = CurrentExplicitTime;
        CreateTickRecordForNode(Context, CurrentSequence, IsLooping(), 0, true);
    }
    bReinitialized = false;
}

void FAnimNode_SequenceEvaluatorBase::Evaluate_AnyThread(FPoseContext& Output) {
    const UAnimSequenceBase* CurrentSequence = GetSequence();
    if (CurrentSequence == nullptr || CurrentSequence->GetSkeleton() == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    FAnimationPoseData AnimationPoseData = Output.GetAnimationPoseData();
    CurrentSequence->GetAnimationPose(AnimationPoseData,
                                      FAnimExtractContext(static_cast<double>(InternalTimeAccumulator), Output.AnimInstanceProxy->ShouldExtractRootMotion(), DeltaTimeRecord, IsLooping()));
}

float FAnimNode_SequenceEvaluatorBase::GetEffectiveDeltaTime(float ExplicitTime, float PrevExplicitTime) const {
    float DeltaTime = ExplicitTime - PrevExplicitTime;
    if (!IsLooping()) return DeltaTime;
    const float PlayLength = GetSequence()->GetPlayLength();
    if (FMath::Abs(DeltaTime) > PlayLength * 0.5f) DeltaTime += DeltaTime > 0.f ? -PlayLength : PlayLength;
    return DeltaTime;
}

}
