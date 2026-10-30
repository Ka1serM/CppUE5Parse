#include "ue5parse/runtime/Engine/Animation/AnimNode_SequencePlayer.h"

#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"

namespace ue5parse::runtime {

namespace {

// A sequence player plays no montage (UE ensures this).
const UAnimSequenceBase* PlayableSequence(const UAnimSequenceBase* Sequence) {
    return Sequence != nullptr && !Sequence->IsA<UAnimMontage>() ? Sequence : nullptr;
}

}

const FInputScaleBiasClampConstants& FAnimNode_SequencePlayerBase::GetPlayRateScaleBiasClampConstants() const {
    static const FInputScaleBiasClampConstants Defaults;
    return Defaults;
}

float FAnimNode_SequencePlayerBase::GetAdjustedPlayRate(float DeltaTime) {
    const float PlayRateBasis = GetPlayRateBasis();
    return PlayRateScaleBiasClampState.ApplyTo(GetPlayRateScaleBiasClampConstants(),
                                               FMath::IsNearlyZero(PlayRateBasis) ? 0.0f : GetPlayRate() / PlayRateBasis, DeltaTime);
}

UAnimationAsset* FAnimNode_SequencePlayerBase::GetAnimAsset() const { return GetSequence(); }

float FAnimNode_SequencePlayerBase::GetCurrentAssetTimePlayRateAdjusted() const {
    const UAnimSequenceBase* Sequence = GetSequence();
    const float PlayRateBasis = GetPlayRateBasis();
    const float AdjustedPlayRate =
        PlayRateScaleBiasClampState.ApplyTo(GetPlayRateScaleBiasClampConstants(), FMath::IsNearlyZero(PlayRateBasis) ? 0.0f : GetPlayRate() / PlayRateBasis);
    const float EffectivePlayRate = (Sequence ? Sequence->RateScale : 1.0f) * AdjustedPlayRate;
    return EffectivePlayRate < 0.0f ? GetCurrentAssetLength() - InternalTimeAccumulator : InternalTimeAccumulator;
}

float FAnimNode_SequencePlayerBase::GetCurrentAssetLength() const {
    const UAnimSequenceBase* Sequence = GetSequence();
    return Sequence ? Sequence->GetPlayLength() : 0.0f;
}

void FAnimNode_SequencePlayerBase::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
    GetEvaluateGraphExposedInputs().Execute(Context);
    InternalTimeAccumulator = GetStartPosition();
    PlayRateScaleBiasClampState.Reinitialize();
    const UAnimSequenceBase* Sequence = PlayableSequence(GetSequence());
    if (Sequence == nullptr) return;
    // A player that starts at 0 and plays backward starts at the end.
    const float EffectivePlayRate = Sequence->RateScale * GetAdjustedPlayRate(0.0f);
    if (InternalTimeAccumulator == 0.0f && EffectivePlayRate < 0.0f) InternalTimeAccumulator = Sequence->GetPlayLength();
}

void FAnimNode_SequencePlayerBase::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    const UAnimSequenceBase* Sequence = PlayableSequence(GetSequence());
    if (Sequence == nullptr || Sequence->GetSkeleton() == nullptr) return;
    InternalTimeAccumulator = FMath::Clamp(InternalTimeAccumulator, 0.0f, Sequence->GetPlayLength());
    CreateTickRecordForNode(Context, Sequence, IsLooping(), GetAdjustedPlayRate(Context.GetDeltaTime()), false);
}

void FAnimNode_SequencePlayerBase::Evaluate_AnyThread(FPoseContext& Output) {
    const UAnimSequenceBase* Sequence = PlayableSequence(GetSequence());
    if (Sequence == nullptr || Sequence->GetSkeleton() == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    FAnimationPoseData AnimationPoseData = Output.GetAnimationPoseData();
    Sequence->GetAnimationPose(AnimationPoseData,
                               FAnimExtractContext(InternalTimeAccumulator, Output.AnimInstanceProxy->ShouldExtractRootMotion(), DeltaTimeRecord, IsLooping()));
}

}
