#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendListBase.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_BlendListBase::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    PerBlendData.Empty();
    PerBlendData.SetNum(BlendPose.Num());
    if (BlendPose.Num() == 0) return;
    PerBlendData[0].Weight = 1.0f;
    for (FPoseLink& Pose : BlendPose) Pose.Initialize(Context);
    if (GetBlendProfile() != nullptr) ReportLoadProblem("anim.blend_profile", FString("a blend list blends without its blend profile"));
    Initialize();
}

void FAnimNode_BlendListBase::Initialize() {
    LastActiveChildIndex = INDEX_NONE;
    const EAlphaBlendOption BlendType = GetBlendType();
    UCurveFloat* CustomBlendCurve = GetCustomBlendCurve();
    for (FBlendData& Data : PerBlendData) {
        Data.Blend.SetBlendTime(0.0f);
        Data.Blend.SetBlendOption(BlendType);
        Data.Blend.SetCustomCurve(CustomBlendCurve);
    }
    PerBlendData[0].Blend.SetAlpha(1.0f);
}

void FAnimNode_BlendListBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    for (FPoseLink& Pose : BlendPose) Pose.CacheBones(Context);
}

void FAnimNode_BlendListBase::Update_AnyThread(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    if (BlendPose.Num() == 0) return;

    const int32 ChildIndex = GetActiveChildIndex();
    if (ChildIndex != LastActiveChildIndex) {
        const bool bLastChildIndexIsInvalid = LastActiveChildIndex == INDEX_NONE;
        const float CurrentWeight = PerBlendData[ChildIndex].Weight;
        const float WeightDifference = FMath::Clamp(FMath::Abs(1.0f - CurrentWeight), 0.0f, 1.0f);
        float RemainingBlendTime = 0.0f;
        if (bLastChildIndexIsInvalid) {
            RemainingBlendTime = 0.0f;
        } else if (GetTransitionType() == EBlendListTransitionType::Inertialization) {
            // As in UE when the graph has no inertialization node: the request is reported and the switch is instant.
            ReportLoadProblem("anim.inertialization", FString("a blend list requests an inertialization without an inertialization node"));
        } else {
            RemainingBlendTime = GetBlendTimes()[ChildIndex] * WeightDifference;
        }
        for (FBlendData& Data : PerBlendData) Data.RemainingTime = RemainingBlendTime;
        if (RemainingBlendTime == 0.0f && !bLastChildIndexIsInvalid) BlendPose[LastActiveChildIndex].Update(Context.FractionalWeight(0.0f));
        for (int32 Index = 0; Index < PerBlendData.Num(); ++Index) {
            FAlphaBlend& Blend = PerBlendData[Index].Blend;
            Blend.SetBlendTime(RemainingBlendTime);
            Blend.SetValueRange(PerBlendData[Index].Weight, Index == ChildIndex ? 1.0f : 0.0f);
        }
        if (GetResetChildOnActivation() && CurrentWeight <= ZERO_ANIMWEIGHT_THRESH)
            BlendPose[ChildIndex].Initialize(FAnimationInitializeContext(Context.AnimInstanceProxy));
        LastActiveChildIndex = ChildIndex;
    }

    float SumWeight = 0.0f;
    for (FBlendData& Data : PerBlendData) {
        Data.Blend.Update(Context.GetDeltaTime());
        Data.Weight = Data.Blend.GetBlendedValue();
        SumWeight += Data.Weight;
    }
    if (SumWeight > ZERO_ANIMWEIGHT_THRESH && FMath::Abs(SumWeight - 1.0f) > ZERO_ANIMWEIGHT_THRESH)
        for (FBlendData& Data : PerBlendData) Data.Weight /= SumWeight;
    for (int32 Index = 0; Index < BlendPose.Num(); ++Index)
        if (PerBlendData[Index].Weight > ZERO_ANIMWEIGHT_THRESH) BlendPose[Index].Update(Context.FractionalWeight(PerBlendData[Index].Weight));
}

void FAnimNode_BlendListBase::Evaluate_AnyThread(FPoseContext& Output) {
    TArray<FPoseContext> Poses;
    TArray<const FCompactPose*> SourcePoses;
    TArray<const FBlendedCurve*> SourceCurves;
    TArray<float> SourceWeights;
    Poses.Reserve(PerBlendData.Num());
    for (int32 Index = 0; Index < PerBlendData.Num(); ++Index) {
        if (PerBlendData[Index].Weight <= ZERO_ANIMWEIGHT_THRESH) continue;
        FPoseContext& Pose = Poses.Emplace_GetRef(Output);
        BlendPose[Index].Evaluate(Pose);
        SourceWeights.Add(PerBlendData[Index].Weight);
    }
    if (Poses.Num() == 0) {
        Output.ResetToRefPose();
        return;
    }
    for (const FPoseContext& Pose : Poses) {
        SourcePoses.Add(&Pose.Pose);
        SourceCurves.Add(&Pose.Curve);
    }
    FAnimationPoseData OutAnimationPoseData = Output.GetAnimationPoseData();
    FAnimationRuntime::BlendPosesTogetherIndirect(SourcePoses, SourceCurves, SourceWeights, OutAnimationPoseData);
}

}
