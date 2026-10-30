#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"

#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"

#include <algorithm>
#include <cfloat>

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"
#include "ue5parse/runtime/Engine/Animation/BlendProfile.h"
#include "ue5parse/runtime/Engine/Animation/BlendSpace1D.h"

// UE's Animation/BlendSpace.cpp (UE 5.3) at run time. Marker based sync, notifies and root motion are not part of the
// runtime: a blend space always ticks by length.

namespace ue5parse::runtime {

namespace {

float SmoothingTimeFromSpeed(float Speed) { return Speed > FLT_EPSILON ? 1.0f / (static_cast<float>(UE_EULERS_NUMBER) * Speed) : 0.0f; }

float SpeedFromSmoothingTime(float SmoothingTime) {
    return SmoothingTime > FLT_EPSILON ? 1.0f / (static_cast<float>(UE_EULERS_NUMBER) * SmoothingTime) : 0.0f;
}

// Moves a weight toward its target: a critically damped spring, or at a constant speed.
void SmoothWeight(float& Output, float& OutputRate, float Input, float InputRate, float Target, float DeltaTime, float Speed, bool bUseEaseInOut) {
    if (Speed <= 0.0f) {
        Output = Target;
        return;
    }
    if (bUseEaseInOut) {
        Output = Input;
        OutputRate = InputRate;
        FMath::CriticallyDampedSmoothing(Output, OutputRate, Target, 0.0f, DeltaTime, SmoothingTimeFromSpeed(Speed));
        return;
    }
    Output = static_cast<float>(FMath::FInterpConstantTo(Input, Target, DeltaTime, Speed));
}

}

UBlendSpace::UBlendSpace(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DimensionIndices = {0, 1};
}

UBlendSpace1D::UBlendSpace1D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DimensionIndices = {0};
}

void UBlendSpace::PostLoad() {
    Super::PostLoad();
    InitializePerBoneBlend();
}

void UBlendSpace::InitializePerBoneBlend() {
    if (PerBoneBlendMode == EBlendSpacePerBoneBlendMode::ManualPerBoneOverride) {
        PerBoneBlendValues = ManualPerBoneOverrides;
    } else {
        PerBoneBlendValues.Empty();
        if (const UBlendProfile* BlendProfile = PerBoneBlendProfile.BlendProfile) {
            const float TargetWeightInterpolationTime = SmoothingTimeFromSpeed(TargetWeightInterpolationSpeedPerSec);
            const float BlendProfileInterpolationTime = SmoothingTimeFromSpeed(PerBoneBlendProfile.TargetWeightInterpolationSpeedPerSec);
            for (const FBlendProfileBoneEntry& BoneEntry : BlendProfile->ProfileEntries) {
                FPerBoneInterpolation BoneInterpolation;
                BoneInterpolation.BoneReference = BoneEntry.BoneReference;
                const float InterpolatedTime =
                    FMath::Lerp(TargetWeightInterpolationTime, BlendProfileInterpolationTime, FMath::Clamp(BoneEntry.BlendScale, 0.0f, 1.0f));
                BoneInterpolation.InterpolationSpeedPerSec = SpeedFromSmoothingTime(InterpolatedTime);
                PerBoneBlendValues.Add(BoneInterpolation);
            }
        }
    }
    for (FPerBoneInterpolation& BoneInterpolationData : PerBoneBlendValues) BoneInterpolationData.Initialize(GetSkeleton());
}

bool UBlendSpace::UpdateBlendSamples_Internal(const FVector& InBlendSpacePosition, float InDeltaTime, TArray<FBlendSampleData>& InOutOldSampleDataList,
                                              TArray<FBlendSampleData>& InOutSampleDataCache, int32& InOutCachedTriangulationIndex) const {
    TArray<FBlendSampleData> NewSampleDataList;
    for (const FBlendSampleData& Sample : InOutSampleDataCache) InOutOldSampleDataList.Add(Sample);
    InOutSampleDataCache.Reset();
    if (!GetSamplesFromBlendInput(InBlendSpacePosition, NewSampleDataList, InOutCachedTriangulationIndex, true)) return false;
    if (TargetWeightInterpolationSpeedPerSec > 0.f || PerBoneBlendValues.Num() > 0) {
        if (InterpolateWeightOfSampleData(InDeltaTime, InOutOldSampleDataList, NewSampleDataList, InOutSampleDataCache))
            FBlendSampleData::NormalizeDataWeight(InOutSampleDataCache);
        else
            InOutSampleDataCache = NewSampleDataList;
        return true;
    }
    InOutSampleDataCache = std::move(NewSampleDataList);
    return true;
}

bool UBlendSpace::UpdateBlendSamples(const FVector& InBlendSpacePosition, float InDeltaTime, TArray<FBlendSampleData>& InOutSampleDataCache,
                                     int32& InOutCachedTriangulationIndex) const {
    TArray<FBlendSampleData> OldSampleDataList;
    return UpdateBlendSamples_Internal(InBlendSpacePosition, InDeltaTime, OldSampleDataList, InOutSampleDataCache, InOutCachedTriangulationIndex);
}

void UBlendSpace::TickAssetPlayer(FAnimTickRecord& Instance, FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const {
    TArray<FBlendSampleData> OldSampleDataList;
    TArray<FBlendSampleData>& SampleDataList = *Instance.BlendSpace.BlendSampleDataCache;
    const float DeltaTime = Context.GetDeltaTime();
    float ExtraNormalizedDeltaTime =
        Instance.DeltaTimeRecord->IsPreviousValid() ? FMath::Wrap(*Instance.TimeAccumulator - Instance.DeltaTimeRecord->Previous, -0.5f, 0.5f) : 0.0f;
    Instance.DeltaTimeRecord->Set(*Instance.TimeAccumulator, Instance.PlayRateMultiplier * DeltaTime);

    const FVector BlendSpacePosition(Instance.BlendSpace.BlendSpacePositionX, Instance.BlendSpace.BlendSpacePositionY, 0.f);
    const FVector FilteredBlendInput = FilterInput(Instance.BlendSpace.BlendFilter, BlendSpacePosition, DeltaTime);
    int32 LocalTriangulationIndex = INDEX_NONE;
    int32& TriangulationIndex = Instance.BlendSpace.TriangulationIndex ? *Instance.BlendSpace.TriangulationIndex : LocalTriangulationIndex;
    if (!UpdateBlendSamples_Internal(FilteredBlendInput, DeltaTime, OldSampleDataList, SampleDataList, TriangulationIndex)) return;

    // With smoothed weights, the length change of the new weights does not change the speed of play.
    const float PreInterpAnimLength = TargetWeightInterpolationSpeedPerSec > 0.f ? GetAnimationLengthFromSampleData(SampleDataList) : 0.f;
    const EBlendSpaceAxis AxisToScale = GetAxisToScale();
    if (AxisToScale != BSA_None) {
        // The scaled axis speeds the play by the ratio of the input to the filtered (and clamped) input.
        float FilterMultiplier = 1.f;
        if (!BlendSpacePosition.Equals(FilteredBlendInput)) {
            if (AxisToScale == BSA_X && FilteredBlendInput.X != 0.f) FilterMultiplier = static_cast<float>(BlendSpacePosition.X / FilteredBlendInput.X);
            else if (AxisToScale == BSA_Y && FilteredBlendInput.Y != 0.f) FilterMultiplier = static_cast<float>(BlendSpacePosition.Y / FilteredBlendInput.Y);
        }
        const FVector ClampedInput = GetClampedBlendInput(FilteredBlendInput);
        if (!ClampedInput.Equals(FilteredBlendInput)) {
            if (AxisToScale == BSA_X && !BlendParameters[0].bWrapInput) {
                if (ClampedInput.X != 0.f) FilterMultiplier *= static_cast<float>(FilteredBlendInput.X / ClampedInput.X);
            } else if (AxisToScale == BSA_Y) {
                if (ClampedInput.Y != 0.f && !BlendParameters[1].bWrapInput) FilterMultiplier *= static_cast<float>(FilteredBlendInput.Y / ClampedInput.Y);
            }
        }
        Instance.DeltaTimeRecord->Delta *= FilterMultiplier;
    }
    const float NewAnimLength = GetAnimationLengthFromSampleData(SampleDataList);
    if (PreInterpAnimLength > 0.f && NewAnimLength > 0.f) Instance.DeltaTimeRecord->Delta *= PreInterpAnimLength / NewAnimLength;

    float& NormalizedCurrentTime = *Instance.TimeAccumulator;
    if (Context.ShouldResyncToSyncGroup() && !Instance.bIsEvaluator) NormalizedCurrentTime = Context.GetAnimationPositionRatio();
    float NormalizedPreviousTime = NormalizedCurrentTime;
    if (Context.IsLeader()) {
        Context.SetPreviousAnimationPositionRatio(NormalizedCurrentTime);
        float CurrentTime = NormalizedCurrentTime * NewAnimLength;
        FAnimationRuntime::AdvanceTime(Instance.bLooping, Instance.DeltaTimeRecord->Delta, CurrentTime, NewAnimLength);
        NormalizedCurrentTime = NewAnimLength != 0.f ? CurrentTime / NewAnimLength : 0.0f;
        Context.SetAnimationPositionRatio(NormalizedCurrentTime);
    } else {
        NormalizedPreviousTime = Context.GetPreviousAnimationPositionRatio();
        NormalizedCurrentTime = Context.GetAnimationPositionRatio();
    }

    // Each sample plays at the same normalized time.
    float ClampedNormalizedPreviousTime = FMath::Clamp(NormalizedPreviousTime, 0.f, 1.f);
    float ClampedNormalizedCurrentTime = FMath::Clamp(NormalizedCurrentTime, 0.f, 1.f);
    if (Instance.bIsEvaluator && !Instance.BlendSpace.bTeleportToTime) {
        ClampedNormalizedPreviousTime = FMath::Wrap(ClampedNormalizedPreviousTime - ExtraNormalizedDeltaTime, 0.0f, 1.0f);
        ClampedNormalizedCurrentTime = FMath::Clamp(NormalizedPreviousTime, 0.f, 1.f);
    }
    FAnimNotifyContext NotifyContext(Instance);
    const bool bGenerateNotifies = NotifyTriggerMode != ENotifyTriggerMode::None;
    // By default only the sample with the highest weight gives notifies.
    const bool bTriggerNotifyHighestWeightedAnim = NotifyTriggerMode == ENotifyTriggerMode::HighestWeightedAnimation && SampleDataList.Num() > 0;
    int32 HighestWeightIndex = INDEX_NONE;
    if (bGenerateNotifies && bTriggerNotifyHighestWeightedAnim) {
        HighestWeightIndex = 0;
        for (int32 I = 1; I < SampleDataList.Num(); I++)
            if (SampleDataList[I].GetClampedWeight() > SampleDataList[HighestWeightIndex].GetClampedWeight()) HighestWeightIndex = I;
    }
    for (int32 I = 0; I < SampleDataList.Num(); ++I) {
        FBlendSampleData& SampleEntry = SampleDataList[I];
        if (!SampleData.IsValidIndex(SampleEntry.SampleDataIndex) || SampleEntry.TotalWeight <= ZERO_ANIMWEIGHT_THRESH) continue;
        const FBlendSample& Sample = SampleData[SampleEntry.SampleDataIndex];
        if (Sample.Animation == nullptr) continue;
        const UAnimSequenceBase* Animation = Sample.Animation;
        const float MultipliedSampleRateScale = Animation->RateScale * Sample.RateScale;
        const float SampleNormalizedPreviousTime = MultipliedSampleRateScale >= 0.f ? ClampedNormalizedPreviousTime : 1.f - ClampedNormalizedPreviousTime;
        const float SampleNormalizedCurrentTime = MultipliedSampleRateScale >= 0.f ? ClampedNormalizedCurrentTime : 1.f - ClampedNormalizedCurrentTime;
        const float PrevSampleDataTime = SampleNormalizedPreviousTime * Animation->GetPlayLength();
        SampleEntry.Time = SampleNormalizedCurrentTime * Animation->GetPlayLength();
        float DeltaTimePosition = SampleEntry.Time - PrevSampleDataTime;
        // A move against the play direction went around the end.
        const float SampleMoveDelta = Instance.DeltaTimeRecord->Delta * MultipliedSampleRateScale;
        if (SampleMoveDelta * DeltaTimePosition < 0.f) DeltaTimePosition += FMath::Sign(SampleMoveDelta) * Animation->GetPlayLength();
        if (bGenerateNotifies && (!bTriggerNotifyHighestWeightedAnim || I == HighestWeightIndex))
            Animation->GetAnimNotifies(PrevSampleDataTime, DeltaTimePosition, NotifyContext);
        SampleEntry.DeltaTimeRecord.Set(PrevSampleDataTime, DeltaTimePosition);
    }
    if (bGenerateNotifies && NotifyContext.ActiveNotifies.Num() > 0)
        NotifyQueue.AddAnimNotifies(Context.ShouldGenerateNotifies(), NotifyContext.ActiveNotifies, Instance.EffectiveBlendWeight);
}

bool UBlendSpace::ContainsMatchingSamples(EAdditiveAnimationType AdditiveType) const {
    bool bMatching = true;
    for (const FBlendSample& Sample : SampleData) {
        const UAnimSequenceBase* Animation = Sample.Animation;
        bMatching &= (SampleData.Num() > 1 && Animation == nullptr) ||
                     (Animation && (AdditiveType == AAT_None || Animation->IsValidAdditive()) && Animation->GetAdditiveAnimType() == AdditiveType);
        if (!bMatching) break;
    }
    return bMatching && SampleData.Num() > 0;
}

bool UBlendSpace::IsValidAdditive() const { return ContainsMatchingSamples(AAT_LocalSpaceBase) || ContainsMatchingSamples(AAT_RotationOffsetMeshSpace); }

void UBlendSpace::ResetToRefPose(FCompactPose& OutPose) const {
    if (IsValidAdditive()) OutPose.ResetToAdditiveIdentity();
    else OutPose.ResetToRefPose();
}

void UBlendSpace::GetPerBoneInterpolationIndices(const FBoneContainer& RequiredBones, TArray<int32>& OutPerBoneIndices) const {
    // The smoothed bones by skeleton index, children first, so that a bone takes its nearest listed parent's entry.
    TArray<int32> SortedOrder;
    SortedOrder.Reserve(PerBoneBlendValues.Num());
    for (int32 Iter = 0; Iter < PerBoneBlendValues.Num(); ++Iter) SortedOrder.Add(Iter);
    std::stable_sort(SortedOrder.begin(), SortedOrder.end(), [this](int32 A, int32 B) {
        return PerBoneBlendValues[A].BoneReference.BoneIndex > PerBoneBlendValues[B].BoneReference.BoneIndex;
    });
    TArray<FCompactPoseBoneIndex> SmoothedBoneCompactIndices;
    SmoothedBoneCompactIndices.Reserve(SortedOrder.Num());
    for (const int32 OriginalIndex : SortedOrder)
        SmoothedBoneCompactIndices.Add(RequiredBones.GetCompactPoseIndexFromSkeletonIndex(PerBoneBlendValues[OriginalIndex].BoneReference.BoneIndex));

    const int32 NumBones = RequiredBones.GetCompactPoseNumBones();
    OutPerBoneIndices.Reset();
    OutPerBoneIndices.SetNum(NumBones);
    for (int32 Index = 0; Index < NumBones; ++Index) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        OutPerBoneIndices[Index] = INDEX_NONE;
        for (int32 Iter = 0; Iter < SortedOrder.Num(); ++Iter) {
            const FCompactPoseBoneIndex SmoothedBone = SmoothedBoneCompactIndices[Iter];
            if (SmoothedBone == BoneIndex || (SmoothedBone.IsValid() && RequiredBones.BoneIsChildOf(BoneIndex, SmoothedBone))) {
                OutPerBoneIndices[Index] = SortedOrder[Iter];
                break;
            }
        }
    }
}

void UBlendSpace::GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, const FAnimExtractContext& ExtractionContext,
                                   FAnimationPoseData& OutAnimationPoseData) const {
    GetAnimationPose(BlendSampleDataCache, {}, nullptr, false, ExtractionContext, OutAnimationPoseData);
}

void UBlendSpace::GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, std::span<FPoseLink> InPoseLinks, FAnimInstanceProxy* InProxy,
                                   bool bInExpectsAdditivePose, const FAnimExtractContext& ExtractionContext, FAnimationPoseData& OutAnimationPoseData) const {
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    if (BlendSampleDataCache.Num() == 0) {
        ResetToRefPose(OutPose);
        return;
    }
    const bool bNested = !InPoseLinks.empty();
    const int32 NumPoses = BlendSampleDataCache.Num();
    TArray<FCompactPose> ChildrenPoses;
    ChildrenPoses.SetNum(NumPoses);
    TArray<FBlendedCurve> ChildrenCurves;
    ChildrenCurves.SetNum(NumPoses);
    TArray<float> ChildrenWeights;
    ChildrenWeights.SetNum(NumPoses);
    for (int32 I = 0; I < NumPoses; ++I) {
        FCompactPose& Pose = ChildrenPoses[I];
        Pose.SetBoneContainer(&OutPose.GetBoneContainer());
        ChildrenWeights[I] = 0.f;
        const int32 SampleIndex = BlendSampleDataCache[I].SampleDataIndex;
        if (!SampleData.IsValidIndex(SampleIndex)) {
            ResetToRefPose(Pose);
            continue;
        }
        const FBlendSample& Sample = SampleData[SampleIndex];
        ChildrenWeights[I] = BlendSampleDataCache[I].GetClampedWeight();
        if (bNested) {
            FPoseContext ChildPoseContext(InProxy, bInExpectsAdditivePose);
            InPoseLinks[SampleIndex].Evaluate(ChildPoseContext);
            Pose.MoveBonesFrom(ChildPoseContext.Pose);
            ChildrenCurves[I] = std::move(ChildPoseContext.Curve);
            continue;
        }
        const UAnimSequenceBase* Animation = Sample.Animation;
        if (Animation == nullptr || Animation->GetSkeleton() == nullptr) {
            ResetToRefPose(Pose);
            continue;
        }
        const float Time = FMath::Clamp(BlendSampleDataCache[I].Time, 0.f, Animation->GetPlayLength());
        FAnimationPoseData ChildAnimationPoseData(Pose, ChildrenCurves[I]);
        Animation->GetAnimationPose(ChildAnimationPoseData, FAnimExtractContext(static_cast<double>(Time), ExtractionContext.bExtractRootMotion,
                                                                                BlendSampleDataCache[I].DeltaTimeRecord, ExtractionContext.bLooping));
    }

    if (PerBoneBlendValues.Num() > 0) {
        TArray<int32> PerBoneIndices;
        GetPerBoneInterpolationIndices(OutPose.GetBoneContainer(), PerBoneIndices);
        if (bAllowMeshSpaceBlending && !bContainsRotationOffsetMeshSpaceSamples)
            FAnimationRuntime::BlendPosesTogetherPerBoneInMeshSpace(ChildrenPoses, ChildrenCurves, PerBoneIndices, BlendSampleDataCache, OutAnimationPoseData);
        else
            FAnimationRuntime::BlendPosesTogetherPerBone(ChildrenPoses, ChildrenCurves, PerBoneIndices, BlendSampleDataCache, OutAnimationPoseData);
    } else {
        TArray<const FCompactPose*> PosePointers;
        TArray<const FBlendedCurve*> CurvePointers;
        PosePointers.Reserve(NumPoses);
        CurvePointers.Reserve(NumPoses);
        for (int32 I = 0; I < NumPoses; ++I) {
            PosePointers.Add(&ChildrenPoses[I]);
            CurvePointers.Add(&ChildrenCurves[I]);
        }
        FAnimationRuntime::BlendPosesTogetherIndirect(PosePointers, CurvePointers, ChildrenWeights, OutAnimationPoseData);
    }
    OutPose.NormalizeRotations();
}

bool UBlendSpace::GetSamplesFromBlendInput(const FVector& BlendInput, TArray<FBlendSampleData>& OutSampleDataList, int32& InOutCachedTriangulationIndex,
                                           bool bCombineAnimations) const {
    OutSampleDataList.Reset();
    if (!bInterpolateUsingGrid) {
        TArray<FWeightedBlendSample> WeightedBlendSamples;
        BlendSpaceData.GetSamples(WeightedBlendSamples, DimensionIndices, GetNormalizedBlendInput(BlendInput), InOutCachedTriangulationIndex);
        for (const FWeightedBlendSample& WeightedBlendSample : WeightedBlendSamples) {
            if (WeightedBlendSample.SampleWeight <= ZERO_ANIMWEIGHT_THRESH) continue;
            FBlendSampleData BlendSampleData;
            BlendSampleData.SampleDataIndex = WeightedBlendSample.SampleIndex;
            BlendSampleData.TotalWeight = WeightedBlendSample.SampleWeight;
            BlendSampleData.Animation = SampleData[WeightedBlendSample.SampleIndex].Animation;
            BlendSampleData.SamplePlayRate = SampleData[WeightedBlendSample.SampleIndex].RateScale;
            OutSampleDataList.Add(BlendSampleData);
        }
    } else {
        TArray<FGridBlendSample> RawGridSamples;
        GetRawSamplesFromBlendInput(BlendInput, RawGridSamples);
        OutSampleDataList.Reserve(RawGridSamples.Num() * FEditorElement::MAX_VERTICES);
        for (const FGridBlendSample& GridSample : RawGridSamples) {
            for (int32 Ind = 0; Ind < FEditorElement::MAX_VERTICES; ++Ind) {
                const int32 SampleDataIndex = GridSample.GridElement.Indices[Ind];
                if (!SampleData.IsValidIndex(SampleDataIndex)) continue;
                FBlendSampleData& NewSampleData = OutSampleDataList[OutSampleDataList.AddUnique(FBlendSampleData(SampleDataIndex))];
                NewSampleData.AddWeight(GridSample.GridElement.Weights[Ind] * GridSample.BlendWeight);
                NewSampleData.Animation = SampleData[SampleDataIndex].Animation;
                NewSampleData.SamplePlayRate = SampleData[SampleDataIndex].RateScale;
            }
        }
    }

    // Two samples of one animation play as one, at their weighted rate.
    if (bCombineAnimations) {
        for (int32 Index1 = 0; Index1 < OutSampleDataList.Num(); ++Index1) {
            for (int32 Index2 = Index1 + 1; Index2 < OutSampleDataList.Num(); ++Index2) {
                FBlendSampleData& FirstSample = OutSampleDataList[Index1];
                FBlendSampleData& SecondSample = OutSampleDataList[Index2];
                if (FirstSample.SampleDataIndex != SecondSample.SampleDataIndex &&
                    (FirstSample.Animation == nullptr || FirstSample.Animation != SecondSample.Animation))
                    continue;
                const float TotalWeight = FirstSample.GetClampedWeight() + SecondSample.GetClampedWeight();
                if (!FMath::IsNearlyZero(TotalWeight)) {
                    if (FirstSample.GetClampedWeight() < SecondSample.GetClampedWeight()) std::swap(FirstSample, SecondSample);
                    const float OriginalWeightedPlayRate = FirstSample.SamplePlayRate * (FirstSample.GetClampedWeight() / TotalWeight);
                    const float SecondSampleWeightedPlayRate = SecondSample.SamplePlayRate * (SecondSample.GetClampedWeight() / TotalWeight);
                    FirstSample.SamplePlayRate = OriginalWeightedPlayRate + SecondSampleWeightedPlayRate;
                    FirstSample.AddWeight(SecondSample.GetClampedWeight());
                }
                OutSampleDataList.RemoveAtSwap(Index2);
                --Index2;
            }
        }
    }

    std::stable_sort(OutSampleDataList.begin(), OutSampleDataList.end(),
                     [](const FBlendSampleData& A, const FBlendSampleData& B) { return B.TotalWeight < A.TotalWeight; });
    float TotalWeight = 0.f;
    for (int32 I = 0; I < OutSampleDataList.Num(); ++I) {
        if (OutSampleDataList[I].TotalWeight < ZERO_ANIMWEIGHT_THRESH) {
            OutSampleDataList.RemoveAt(I, OutSampleDataList.Num() - I);
            break;
        }
        TotalWeight += OutSampleDataList[I].TotalWeight;
    }
    for (FBlendSampleData& Sample : OutSampleDataList) Sample.TotalWeight /= TotalWeight;
    return OutSampleDataList.Num() != 0;
}

void UBlendSpace::InitializeFilter(FBlendFilter* Filter, int32 NumDimensions) const {
    if (Filter == nullptr) return;
    Filter->FilterPerAxis.SetNum(NumDimensions);
    for (int32 FilterIndex = 0; FilterIndex != NumDimensions; ++FilterIndex) {
        const FInterpolationParameter& Param = InterpolationParam[FilterIndex];
        Filter->FilterPerAxis[FilterIndex].Initialize(Param.InterpolationTime, Param.InterpolationType, Param.DampingRatio, BlendParameters[FilterIndex].Min,
                                                      BlendParameters[FilterIndex].Max, Param.MaxSpeed, !BlendParameters[FilterIndex].bWrapInput);
    }
}

void UBlendSpace::UpdateFilterParams(FBlendFilter* Filter) const {
    if (Filter == nullptr) return;
    for (int32 FilterIndex = 0; FilterIndex != Filter->FilterPerAxis.Num(); ++FilterIndex)
        Filter->FilterPerAxis[FilterIndex].SetParams(InterpolationParam[FilterIndex].DampingRatio, BlendParameters[FilterIndex].Min,
                                                     BlendParameters[FilterIndex].Max, InterpolationParam[FilterIndex].MaxSpeed,
                                                     !BlendParameters[FilterIndex].bWrapInput);
}

float UBlendSpace::GetAnimationLengthFromSampleData(const TArray<FBlendSampleData>& SampleDataList) const {
    float BlendAnimLength = 0.f;
    for (const FBlendSampleData& SampleEntry : SampleDataList) {
        if (!SampleData.IsValidIndex(SampleEntry.SampleDataIndex)) continue;
        const UAnimSequenceBase* Animation = SampleData[SampleEntry.SampleDataIndex].Animation;
        if (Animation == nullptr) continue;
        const float MultipliedSampleRateScale = Animation->RateScale * SampleEntry.SamplePlayRate;
        BlendAnimLength += Animation->GetPlayLength() / (MultipliedSampleRateScale != 0.0f ? FMath::Abs(MultipliedSampleRateScale) : 1.0f) *
                           SampleEntry.GetClampedWeight();
    }
    return BlendAnimLength;
}

FVector UBlendSpace::GetClampedBlendInput(const FVector& BlendInput) const {
    FVector AdjustedInput = BlendInput;
    for (int32 Axis = 0; Axis != 3; ++Axis)
        if (!BlendParameters[Axis].bWrapInput) AdjustedInput[Axis] = FMath::Clamp(AdjustedInput[Axis], double(BlendParameters[Axis].Min), double(BlendParameters[Axis].Max));
    return AdjustedInput;
}

FVector UBlendSpace::GetClampedAndWrappedBlendInput(const FVector& BlendInput) const {
    FVector AdjustedInput = BlendInput;
    for (int32 Axis = 0; Axis != 3; ++Axis) {
        const double Min = BlendParameters[Axis].Min;
        const double Max = BlendParameters[Axis].Max;
        AdjustedInput[Axis] = BlendParameters[Axis].bWrapInput ? FMath::Wrap(AdjustedInput[Axis], Min, Max) : FMath::Clamp(AdjustedInput[Axis], Min, Max);
    }
    return AdjustedInput;
}

FVector UBlendSpace::ConvertBlendInputToGridSpace(const FVector& BlendInput) const {
    const FVector MinBlendInput(BlendParameters[0].Min, BlendParameters[1].Min, BlendParameters[2].Min);
    const FVector GridSize(BlendParameters[0].GetGridSize(), BlendParameters[1].GetGridSize(), BlendParameters[2].GetGridSize());
    return (GetClampedAndWrappedBlendInput(BlendInput) - MinBlendInput) / GridSize;
}

FVector UBlendSpace::GetNormalizedBlendInput(const FVector& BlendInput) const {
    const FVector MinBlendInput(BlendParameters[0].Min, BlendParameters[1].Min, BlendParameters[2].Min);
    const FVector MaxBlendInput(BlendParameters[0].Max, BlendParameters[1].Max, BlendParameters[2].Max);
    return (GetClampedAndWrappedBlendInput(BlendInput) - MinBlendInput) / (MaxBlendInput - MinBlendInput);
}

bool UBlendSpace::InterpolateWeightOfSampleData(float DeltaTime, const TArray<FBlendSampleData>& OldSampleDataList,
                                                const TArray<FBlendSampleData>& NewSampleDataList, TArray<FBlendSampleData>& FinalSampleDataList) const {
    const int32 NumPerBone = PerBoneBlendValues.Num();
    float TotalFinalWeight = 0.f;
    float TotalFinalPerBoneWeight = 0.0f;
    auto InitPerBone = [NumPerBone](FBlendSampleData& Sample) {
        if (Sample.PerBoneBlendData.Num() == NumPerBone) return;
        Sample.PerBoneBlendData.SetNum(NumPerBone);
        Sample.PerBoneWeightRate.SetNum(NumPerBone);
        for (int32 Iter = 0; Iter < NumPerBone; ++Iter) {
            Sample.PerBoneBlendData[Iter] = Sample.TotalWeight;
            Sample.PerBoneWeightRate[Iter] = Sample.WeightRate;
        }
    };
    auto AddIfRelevant = [&](const FBlendSampleData& InterpData, float TotalPerBoneWeight) {
        if (InterpData.TotalWeight <= ZERO_ANIMWEIGHT_THRESH && TotalPerBoneWeight <= ZERO_ANIMWEIGHT_THRESH) return false;
        FinalSampleDataList.Add(InterpData);
        TotalFinalWeight += InterpData.GetClampedWeight();
        TotalFinalPerBoneWeight += TotalPerBoneWeight;
        return true;
    };

    // The old samples move to their new weight, or to 0 when they are gone.
    for (FBlendSampleData OldSample : OldSampleDataList) {
        InitPerBone(OldSample);
        bool bTargetSampleExists = false;
        for (const FBlendSampleData& NewSample : NewSampleDataList) {
            if (NewSample.SampleDataIndex != OldSample.SampleDataIndex) continue;
            FBlendSampleData InterpData = NewSample;
            SmoothWeight(InterpData.TotalWeight, InterpData.WeightRate, OldSample.TotalWeight, OldSample.WeightRate, NewSample.TotalWeight, DeltaTime,
                         TargetWeightInterpolationSpeedPerSec, bTargetWeightInterpolationEaseInOut);
            InterpData.PerBoneBlendData = OldSample.PerBoneBlendData;
            InterpData.PerBoneWeightRate = OldSample.PerBoneWeightRate;
            float TotalPerBoneWeight = 0.0f;
            for (int32 Iter = 0; Iter < InterpData.PerBoneBlendData.Num(); ++Iter) {
                SmoothWeight(InterpData.PerBoneBlendData[Iter], InterpData.PerBoneWeightRate[Iter], OldSample.PerBoneBlendData[Iter],
                             OldSample.PerBoneWeightRate[Iter], NewSample.TotalWeight, DeltaTime, PerBoneBlendValues[Iter].InterpolationSpeedPerSec,
                             bTargetWeightInterpolationEaseInOut);
                TotalPerBoneWeight += InterpData.PerBoneBlendData[Iter];
            }
            if (AddIfRelevant(InterpData, TotalPerBoneWeight)) {
                bTargetSampleExists = true;
                break;
            }
        }
        if (bTargetSampleExists) continue;
        FBlendSampleData InterpData = OldSample;
        SmoothWeight(InterpData.TotalWeight, InterpData.WeightRate, OldSample.TotalWeight, OldSample.WeightRate, 0.0f, DeltaTime,
                     TargetWeightInterpolationSpeedPerSec, bTargetWeightInterpolationEaseInOut);
        float TotalPerBoneWeight = 0.0f;
        for (int32 Iter = 0; Iter < InterpData.PerBoneBlendData.Num(); ++Iter) {
            SmoothWeight(InterpData.PerBoneBlendData[Iter], InterpData.PerBoneWeightRate[Iter], OldSample.PerBoneBlendData[Iter],
                         OldSample.PerBoneWeightRate[Iter], 0.0f, DeltaTime, PerBoneBlendValues[Iter].InterpolationSpeedPerSec,
                         bTargetWeightInterpolationEaseInOut);
            TotalPerBoneWeight += InterpData.PerBoneBlendData[Iter];
        }
        AddIfRelevant(InterpData, TotalPerBoneWeight);
    }

    // The new samples start from 0.
    for (FBlendSampleData NewSample : NewSampleDataList) {
        InitPerBone(NewSample);
        bool bAlreadyAdded = false;
        for (const FBlendSampleData& FinalSample : FinalSampleDataList)
            if (FinalSample.SampleDataIndex == NewSample.SampleDataIndex) {
                bAlreadyAdded = true;
                break;
            }
        if (bAlreadyAdded) continue;
        FBlendSampleData InterpData = NewSample;
        const float TargetWeight = InterpData.TotalWeight;
        SmoothWeight(InterpData.TotalWeight, InterpData.WeightRate, 0.0f, 0.0f, TargetWeight, DeltaTime, TargetWeightInterpolationSpeedPerSec,
                     bTargetWeightInterpolationEaseInOut);
        float TotalPerBoneWeight = 0.0f;
        for (int32 Iter = 0; Iter < InterpData.PerBoneBlendData.Num(); ++Iter) {
            const float Target = NewSample.PerBoneBlendData[Iter];
            SmoothWeight(InterpData.PerBoneBlendData[Iter], InterpData.PerBoneWeightRate[Iter], 0.0f, 0.0f, Target, DeltaTime,
                         PerBoneBlendValues[Iter].InterpolationSpeedPerSec, bTargetWeightInterpolationEaseInOut);
            TotalPerBoneWeight += InterpData.PerBoneBlendData[Iter];
        }
        AddIfRelevant(InterpData, TotalPerBoneWeight);
    }
    return TotalFinalWeight > ZERO_ANIMWEIGHT_THRESH || TotalFinalPerBoneWeight > ZERO_ANIMWEIGHT_THRESH;
}

FVector UBlendSpace::FilterInput(FBlendFilter* Filter, const FVector& BlendInput, float DeltaTime) const {
    FVector FilteredBlendInput = BlendInput;
    if (Filter == nullptr) return FilteredBlendInput;
    for (int32 AxisIndex = 0; AxisIndex < Filter->FilterPerAxis.Num(); ++AxisIndex) {
        if (BlendParameters[AxisIndex].bWrapInput)
            Filter->FilterPerAxis[AxisIndex].WrapToValue(static_cast<float>(BlendInput[AxisIndex]),
                                                         BlendParameters[AxisIndex].Max - BlendParameters[AxisIndex].Min);
        FilteredBlendInput[AxisIndex] = Filter->FilterPerAxis[AxisIndex].UpdateAndGetFilteredData(static_cast<float>(BlendInput[AxisIndex]), DeltaTime);
    }
    return FilteredBlendInput;
}

void UBlendSpace::GetRawSamplesFromBlendInput(const FVector& BlendInput, TArray<FGridBlendSample>& OutBlendSamples) const {
    switch (DimensionIndices.Num()) {
        case 1: GetRawSamplesFromBlendInput1D(BlendInput, OutBlendSamples); break;
        case 2: GetRawSamplesFromBlendInput2D(BlendInput, OutBlendSamples); break;
        default: break;
    }
}

void UBlendSpace::GetRawSamplesFromBlendInput1D(const FVector& BlendInput, TArray<FGridBlendSample>& OutBlendSamples) const {
    const int32 Index0 = DimensionIndices[0];
    const FVector NormalizedBlendInput = ConvertBlendInputToGridSpace(BlendInput);
    const float GridIndex = static_cast<float>(FMath::TruncToDouble(NormalizedBlendInput[Index0]));
    const float Remainder = static_cast<float>(NormalizedBlendInput[Index0]) - GridIndex;
    auto AddElement = [&](int32 Index, float Weight) {
        FGridBlendSample NewSample;
        if (const FEditorElement* Element = GetGridSampleInternal(Index)) {
            NewSample.GridElement = *Element;
            NewSample.BlendWeight = Weight;
        }
        OutBlendSamples.Add(NewSample);
    };
    AddElement(static_cast<int32>(GridIndex), 1.f - Remainder);
    AddElement(static_cast<int32>(GridIndex) + 1, Remainder);
}

void UBlendSpace::GetRawSamplesFromBlendInput2D(const FVector& BlendInput, TArray<FGridBlendSample>& OutBlendSamples) const {
    OutBlendSamples.Reset();
    OutBlendSamples.SetNum(4);
    const FVector NormalizedBlendInput = ConvertBlendInputToGridSpace(BlendInput);
    const int32 GridX = static_cast<int32>(FMath::TruncToDouble(NormalizedBlendInput.X));
    const int32 GridY = static_cast<int32>(FMath::TruncToDouble(NormalizedBlendInput.Y));
    const float RemainderX = static_cast<float>(NormalizedBlendInput.X - GridX);
    const float RemainderY = static_cast<float>(NormalizedBlendInput.Y - GridY);
    auto SetElement = [this](FGridBlendSample& Sample, int32 X, int32 Y, float Weight) {
        if (const FEditorElement* Element = GetEditorElement(X, Y)) {
            Sample.GridElement = *Element;
            Sample.BlendWeight = Weight;
            return;
        }
        Sample = FGridBlendSample();
    };
    // Left bottom, right bottom, left top, right top.
    SetElement(OutBlendSamples[0], GridX, GridY, (1.f - RemainderX) * (1.f - RemainderY));
    SetElement(OutBlendSamples[1], GridX + 1, GridY, RemainderX * (1.f - RemainderY));
    SetElement(OutBlendSamples[2], GridX, GridY + 1, (1.f - RemainderX) * RemainderY);
    SetElement(OutBlendSamples[3], GridX + 1, GridY + 1, RemainderX * RemainderY);
}

void FBlendSpaceData::GetSamples(TArray<FWeightedBlendSample>& OutWeightedSamples, const TArray<int32>& InDimensionIndices,
                                 const FVector& InNormalizedSamplePosition, int32& InOutTriangulationIndex) const {
    switch (InDimensionIndices.Num()) {
        case 1: GetSamples1D(OutWeightedSamples, InDimensionIndices, InNormalizedSamplePosition, InOutTriangulationIndex); return;
        case 2: GetSamples2D(OutWeightedSamples, InDimensionIndices, InNormalizedSamplePosition, InOutTriangulationIndex); return;
        default: OutWeightedSamples.Reset(); return;
    }
}

void FBlendSpaceData::GetSamples1D(TArray<FWeightedBlendSample>& OutWeightedSamples, const TArray<int32>& InDimensionIndices,
                                   const FVector& InNormalizedSamplePosition, int32& InOutSegmentIndex) const {
    OutWeightedSamples.Reset();
    if (Segments.Num() == 0) return;
    if (Segments.Num() == 1 && Segments[0].SampleIndices[0] == Segments[0].SampleIndices[1]) {
        OutWeightedSamples.Add(FWeightedBlendSample(Segments[0].SampleIndices[0], 1.0f));
        return;
    }
    const float P = static_cast<float>(InNormalizedSamplePosition[InDimensionIndices[0]]);
    if (InOutSegmentIndex < 0 || InOutSegmentIndex >= Segments.Num()) InOutSegmentIndex = Segments.Num() / 2;
    for (int32 Attempt = 0; Attempt != Segments.Num(); ++Attempt) {
        const FBlendSpaceSegment& Segment = Segments[InOutSegmentIndex];
        if (P < Segment.Vertices[0]) {
            if (InOutSegmentIndex > 0) {
                --InOutSegmentIndex;
                continue;
            }
            OutWeightedSamples.Add(FWeightedBlendSample(Segment.SampleIndices[0], 1.0f));
            return;
        }
        if (P > Segment.Vertices[1]) {
            if (InOutSegmentIndex < Segments.Num() - 1) {
                ++InOutSegmentIndex;
                continue;
            }
            OutWeightedSamples.Add(FWeightedBlendSample(Segment.SampleIndices[1], 1.0f));
            return;
        }
        const float Frac = FMath::Clamp((P - Segment.Vertices[0]) / (Segment.Vertices[1] - Segment.Vertices[0]), 0.0f, 1.0f);
        OutWeightedSamples.Add(FWeightedBlendSample(Segment.SampleIndices[0], 1.0f - Frac));
        OutWeightedSamples.Add(FWeightedBlendSample(Segment.SampleIndices[1], Frac));
        return;
    }
}

void FBlendSpaceData::GetSamples2D(TArray<FWeightedBlendSample>& OutWeightedSamples, const TArray<int32>& InDimensionIndices,
                                   const FVector& InNormalizedSamplePosition, int32& InOutTriangleIndex) const {
    if (Triangles.Num() == 0) return;
    OutWeightedSamples.Reset();
    const FVector2D P(InNormalizedSamplePosition[InDimensionIndices[0]], InNormalizedSamplePosition[InDimensionIndices[1]]);
    // One sample, or two samples on a line.
    if (Triangles.Num() == 1) {
        const FBlendSpaceTriangle& Triangle = Triangles[0];
        if (Triangle.SampleIndices[0] == Triangle.SampleIndices[1]) {
            OutWeightedSamples.Add(FWeightedBlendSample(Triangle.SampleIndices[0], 1.0f));
            return;
        }
        if (Triangle.SampleIndices[1] == Triangle.SampleIndices[2]) {
            const FVector2D Delta = Triangle.Vertices[1] - Triangle.Vertices[0];
            const float T = static_cast<float>(((P - Triangle.Vertices[0]) | Delta) / (Delta | Delta));
            if (T < 0) {
                OutWeightedSamples.Add(FWeightedBlendSample(Triangle.SampleIndices[0], 1.0f));
            } else if (T > 1.0f) {
                OutWeightedSamples.Add(FWeightedBlendSample(Triangle.SampleIndices[1], 1.0f));
            } else {
                OutWeightedSamples.Add(FWeightedBlendSample(Triangle.SampleIndices[0], 1.0f - T));
                OutWeightedSamples.Add(FWeightedBlendSample(Triangle.SampleIndices[1], T));
            }
            return;
        }
    }

    if (InOutTriangleIndex < 0 || InOutTriangleIndex >= Triangles.Num()) InOutTriangleIndex = Triangles.Num() / 2;
    for (int32 Attempt = 0; Attempt != Triangles.Num(); ++Attempt) {
        const FBlendSpaceTriangle* Triangle = &Triangles[InOutTriangleIndex];
        // The edge the point is farthest outside of; none when the point is inside.
        float LargestDistance = UE_KINDA_SMALL_NUMBER;
        int32 LargestEdgeIndex = -1;
        for (int32 VertexIndex = 0; VertexIndex != FBlendSpaceTriangle::NUM_VERTICES; ++VertexIndex) {
            const float Distance = static_cast<float>((P - Triangle->Vertices[VertexIndex]) | Triangle->EdgeInfo[VertexIndex].Normal);
            if (Distance <= LargestDistance) continue;
            LargestDistance = Distance;
            LargestEdgeIndex = VertexIndex;
        }
        if (LargestEdgeIndex < 0) {
            const FVector Weights = FMath::GetBaryCentric2D(P, Triangle->Vertices[0], Triangle->Vertices[1], Triangle->Vertices[2]);
            for (int32 VertexIndex = 0; VertexIndex != FBlendSpaceTriangle::NUM_VERTICES; ++VertexIndex)
                OutWeightedSamples.Add(FWeightedBlendSample(Triangle->SampleIndices[VertexIndex], static_cast<float>(Weights[VertexIndex])));
            return;
        }
        if (Triangle->EdgeInfo[LargestEdgeIndex].NeighbourTriangleIndex >= 0) {
            InOutTriangleIndex = Triangle->EdgeInfo[LargestEdgeIndex].NeighbourTriangleIndex;
            continue;
        }

        // Outside the perimeter: walk along it to the edge the point projects onto.
        int32 OrigDir = -1;
        int32 StartIndex = LargestEdgeIndex;
        int32 EndIndex = (StartIndex + 1) % FBlendSpaceTriangle::NUM_VERTICES;
        int32 PrevTriangleIndex = InOutTriangleIndex;
        for (int32 PerimeterAttempt = 0; PerimeterAttempt != Triangles.Num(); ++PerimeterAttempt) {
            const FBlendSpaceTriangleEdgeInfo& EdgeInfo = Triangle->EdgeInfo[StartIndex];
            const FVector2D Start = Triangle->Vertices[StartIndex];
            const FVector2D StartToEnd = Triangle->Vertices[EndIndex] - Start;
            const float T = static_cast<float>((StartToEnd | (P - Start)) / (StartToEnd | StartToEnd));
            if (T >= 0.0f && T <= 1.0f) {
                OutWeightedSamples.Add(FWeightedBlendSample(Triangle->SampleIndices[StartIndex], 1.0f - T));
                OutWeightedSamples.Add(FWeightedBlendSample(Triangle->SampleIndices[EndIndex], T));
                return;
            }
            if (OrigDir == -1) OrigDir = T > 1.0f ? 1 : 0;
            const int32 Dir = T > 1.0f ? 1 : 0;
            auto PushCorner = [&]() {
                if (Dir == 0) OutWeightedSamples.Add(FWeightedBlendSample(Triangle->SampleIndices[StartIndex], 1.0f - T));
                else OutWeightedSamples.Add(FWeightedBlendSample(Triangle->SampleIndices[EndIndex], T));
            };
            // The walk turned back: the point is beyond a corner.
            if (OrigDir != Dir) {
                PushCorner();
                InOutTriangleIndex = PrevTriangleIndex;
                return;
            }
            PrevTriangleIndex = InOutTriangleIndex;
            InOutTriangleIndex = EdgeInfo.AdjacentPerimeterTriangleIndices[Dir];
            if (InOutTriangleIndex < 0) {
                PushCorner();
                InOutTriangleIndex = PrevTriangleIndex;
                return;
            }
            Triangle = &Triangles[InOutTriangleIndex];
            StartIndex = EdgeInfo.AdjacentPerimeterVertexIndices[Dir];
            EndIndex = (StartIndex + 1) % FBlendSpaceTriangle::NUM_VERTICES;
        }
    }
}

}
