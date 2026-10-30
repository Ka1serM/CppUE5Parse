#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpacePlayer.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"

namespace ue5parse::runtime {

float FAnimNode_BlendSpacePlayerBase::GetCurrentAssetTime() const {
    const FBlendSampleData* HighestWeightedSample = GetHighestWeightedSample();
    return HighestWeightedSample ? HighestWeightedSample->Time : 0.0f;
}

float FAnimNode_BlendSpacePlayerBase::GetCurrentAssetTimePlayRateAdjusted() const {
    const float Length = GetCurrentAssetLength();
    return GetPlayRate() < 0.0f ? Length - InternalTimeAccumulator * Length : Length * InternalTimeAccumulator;
}

float FAnimNode_BlendSpacePlayerBase::GetCurrentAssetLength() const {
    const FBlendSampleData* HighestWeightedSample = GetHighestWeightedSample();
    const UBlendSpace* CurrentBlendSpace = GetBlendSpace();
    if (HighestWeightedSample == nullptr || CurrentBlendSpace == nullptr) return 0.0f;
    const UAnimSequenceBase* Animation = CurrentBlendSpace->GetBlendSample(HighestWeightedSample->SampleDataIndex).Animation;
    return Animation ? Animation->GetPlayLength() : 0.0f;
}

void FAnimNode_BlendSpacePlayerBase::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
    GetEvaluateGraphExposedInputs().Execute(Context);
    Reinitialize();
    PreviousBlendSpace = GetBlendSpace();
}

void FAnimNode_BlendSpacePlayerBase::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    UpdateInternal(Context);
}

void FAnimNode_BlendSpacePlayerBase::UpdateInternal(const FAnimationUpdateContext& Context) {
    UBlendSpace* CurrentBlendSpace = GetBlendSpace();
    if (CurrentBlendSpace == nullptr || CurrentBlendSpace->GetSkeleton() == nullptr) return;
    if (PreviousBlendSpace != CurrentBlendSpace) Reinitialize(ShouldResetPlayTimeWhenBlendSpaceChanges());
    const FVector Position = GetPosition();

    FAnimTickRecord TickRecord(CurrentBlendSpace, IsLooping(), GetPlayRate(), IsEvaluator(), Context.GetFinalBlendWeight(), InternalTimeAccumulator);
    TickRecord.BlendSpace.BlendSpacePositionX = static_cast<float>(Position.X);
    TickRecord.BlendSpace.BlendSpacePositionY = static_cast<float>(Position.Y);
    TickRecord.BlendSpace.BlendSampleDataCache = &BlendSampleDataCache;
    TickRecord.BlendSpace.BlendFilter = &BlendFilter;
    TickRecord.BlendSpace.TriangulationIndex = &CachedTriangulationIndex;
    TickRecord.BlendSpace.bTeleportToTime = ShouldTeleportToTime();
    AddTickRecord(Context, TickRecord);
    PreviousBlendSpace = CurrentBlendSpace;
}

void FAnimNode_BlendSpacePlayerBase::Evaluate_AnyThread(FPoseContext& Output) {
    const UBlendSpace* CurrentBlendSpace = GetBlendSpace();
    if (CurrentBlendSpace == nullptr || CurrentBlendSpace->GetSkeleton() == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    FAnimationPoseData AnimationPoseData = Output.GetAnimationPoseData();
    CurrentBlendSpace->GetAnimationPose(BlendSampleDataCache, FAnimExtractContext(static_cast<double>(InternalTimeAccumulator), Output.AnimInstanceProxy->ShouldExtractRootMotion(), DeltaTimeRecord, IsLooping()),
                                        AnimationPoseData);
}

void FAnimNode_BlendSpacePlayerBase::SnapToPosition(const FVector& NewPosition) {
    const int32 NumAxis = FMath::Min(BlendFilter.FilterPerAxis.Num(), 3);
    for (int32 Idx = 0; Idx < NumAxis; Idx++) BlendFilter.FilterPerAxis[Idx].SetToValue(static_cast<float>(NewPosition[Idx]));
}

const FBlendSampleData* FAnimNode_BlendSpacePlayerBase::GetHighestWeightedSample() const {
    if (BlendSampleDataCache.Num() == 0) return nullptr;
    const FBlendSampleData* HighestSample = &BlendSampleDataCache[0];
    for (int32 Idx = 1; Idx < BlendSampleDataCache.Num(); ++Idx)
        if (BlendSampleDataCache[Idx].TotalWeight > HighestSample->TotalWeight) HighestSample = &BlendSampleDataCache[Idx];
    return HighestSample;
}

void FAnimNode_BlendSpacePlayerBase::Reinitialize(bool bResetTime) {
    BlendSampleDataCache.Empty();
    if (bResetTime) {
        const float CurrentStartPosition = GetStartPosition();
        InternalTimeAccumulator = FMath::Clamp(CurrentStartPosition, 0.f, 1.0f);
        // A player that starts at 0 and plays backward starts at the end.
        if (CurrentStartPosition == 0.f && GetPlayRate() < 0.0f) InternalTimeAccumulator = 1.0f;
    }
    if (const UBlendSpace* CurrentBlendSpace = GetBlendSpace()) CurrentBlendSpace->InitializeFilter(&BlendFilter);
}

}
