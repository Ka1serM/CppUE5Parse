#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpaceGraphBase.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_BlendSpaceGraphBase::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    UE5PARSE_CHECK(BlendSpace != nullptr);
    BlendSampleDataCache.Empty();
    BlendSpace->InitializeFilter(&BlendFilter);
    for (FPoseLink& SamplePose : SamplePoseLinks) SamplePose.Initialize(Context);
}

void FAnimNode_BlendSpaceGraphBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    for (FPoseLink& SamplePose : SamplePoseLinks) SamplePose.CacheBones(Context);
}

void FAnimNode_BlendSpaceGraphBase::UpdateInternal(const FAnimationUpdateContext& Context) {
    const float DeltaTime = Context.GetDeltaTime();
    const FVector FilteredBlendParams = BlendSpace->FilterInput(&BlendFilter, GetPosition(), DeltaTime);
    BlendSpace->UpdateBlendSamples(FilteredBlendParams, DeltaTime, BlendSampleDataCache, CachedTriangulationIndex);
    for (const FBlendSampleData& Sample : BlendSampleDataCache)
        SamplePoseLinks[Sample.SampleDataIndex].Update(Context.FractionalWeight(Sample.TotalWeight));
}

void FAnimNode_BlendSpaceGraphBase::Update_AnyThread(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    UpdateInternal(Context);
}

void FAnimNode_BlendSpaceGraphBase::Evaluate_AnyThread(FPoseContext& Output) {
    FAnimationPoseData AnimationPoseData = Output.GetAnimationPoseData();
    BlendSpace->GetAnimationPose(BlendSampleDataCache, SamplePoseLinks, Output.AnimInstanceProxy, Output.ExpectsAdditivePose(), FAnimExtractContext(),
                                 AnimationPoseData);
}

void FAnimNode_BlendSpaceGraphBase::SnapToPosition(const FVector& NewPosition) {
    const int32 NumAxis = FMath::Min(BlendFilter.FilterPerAxis.Num(), 3);
    for (int32 Idx = 0; Idx < NumAxis; Idx++) BlendFilter.FilterPerAxis[Idx].SetToValue(static_cast<float>(NewPosition[Idx]));
}

}
