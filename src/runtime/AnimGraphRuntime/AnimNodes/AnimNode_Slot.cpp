#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_Slot.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_Slot::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    Source.Initialize(Context);
    WeightData = FSlotNodeWeightInfo();
    if (SlotNodeInitializationCounter.IsSynchronized_Counter(Context.AnimInstanceProxy->GetSlotNodeInitializationCounter())) return;
    SlotNodeInitializationCounter.SynchronizeWith(Context.AnimInstanceProxy->GetSlotNodeInitializationCounter());
    Context.AnimInstanceProxy->RegisterSlotNodeWithAnimInstance(SlotName);
}

void FAnimNode_Slot::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) { Source.CacheBones(Context); }

void FAnimNode_Slot::Update_AnyThread(const FAnimationUpdateContext& Context) {
    Context.AnimInstanceProxy->GetSlotWeight(SlotName, WeightData.SlotNodeWeight, WeightData.SourceWeight, WeightData.TotalNodeWeight);
    Context.AnimInstanceProxy->UpdateSlotNodeWeight(SlotName, WeightData.SlotNodeWeight, Context.GetFinalBlendWeight());
    if (!bAlwaysUpdateSourcePose && !FAnimWeight::IsRelevant(WeightData.SourceWeight)) return;
    Source.Update(Context.FractionalWeight(FMath::Max(FAnimWeight::GetSmallestRelevantWeight(), WeightData.SourceWeight)));
}

void FAnimNode_Slot::Evaluate_AnyThread(FPoseContext& Output) {
    if (WeightData.SlotNodeWeight <= ZERO_ANIMWEIGHT_THRESH) {
        Source.Evaluate(Output);
        return;
    }
    FPoseContext SourceContext(Output);
    if (WeightData.SourceWeight > ZERO_ANIMWEIGHT_THRESH) Source.Evaluate(SourceContext);
    const FAnimationPoseData SourcePoseData = SourceContext.GetAnimationPoseData();
    FAnimationPoseData OutputPoseData = Output.GetAnimationPoseData();
    Output.AnimInstanceProxy->SlotEvaluatePose(SlotName, SourcePoseData, WeightData.SourceWeight, OutputPoseData, WeightData.SlotNodeWeight,
                                               WeightData.TotalNodeWeight);
}

}
