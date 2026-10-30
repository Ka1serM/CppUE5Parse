#include "ue5parse/runtime/Engine/Animation/AnimSingleNodeInstance.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimComposite.h"
#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/SingleAnimationPlayData.h"

// UE's AnimSingleNodeInstance.cpp, AnimSingleNodeInstanceProxy.cpp and SingleAnimationPlayData.cpp (UE 5.3), without
// the editor preview (preview base poses, preview curves and pose assets, mirroring).

namespace ue5parse::runtime {

void FSingleAnimationPlayData::Initialize(UAnimSingleNodeInstance* Instance) {
    Instance->SetAnimationAsset(AnimToPlay);
    Instance->SetPosition(SavedPosition, false);
    Instance->SetPlayRate(SavedPlayRate);
    Instance->SetPlaying(bSavedPlaying);
    Instance->SetLooping(bSavedLooping);
}

void FSingleAnimationPlayData::PopulateFrom(UAnimSingleNodeInstance* Instance) {
    AnimToPlay = Instance->GetCurrentAsset();
    SavedPosition = Instance->GetCurrentTime();
    SavedPlayRate = Instance->GetPlayRate();
    bSavedPlaying = Instance->IsPlaying();
    bSavedLooping = Instance->IsLooping();
}

void FSingleAnimationPlayData::ValidatePosition() {
    float Max = 0;
    if (const auto* SequenceBase = Cast<UAnimSequenceBase>(AnimToPlay)) Max = SequenceBase->GetPlayLength();
    SavedPosition = FMath::Clamp(SavedPosition, 0.0f, Max);
}

// The proxy.

void FAnimSingleNodeInstanceProxy::Initialize(UAnimInstance* InAnimInstance) {
    FAnimInstanceProxy::Initialize(InAnimInstance);
    CurrentAsset = nullptr;
    BlendSpacePosition = FVector(0, 0, 0);
    CurrentTime = 0.f;
    DeltaTimeRecord = FDeltaTimeRecord();
    FAnimationInitializeContext InitContext(this);
    SingleNode.Initialize_AnyThread(InitContext);
}

void FAnimSingleNodeInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance) {
    FAnimInstanceProxy::InitializeObjects(InAnimInstance);
    CurrentAsset = static_cast<UAnimSingleNodeInstance*>(InAnimInstance)->CurrentAsset;
}

bool FAnimSingleNodeInstanceProxy::Evaluate(FPoseContext& Output) {
    SingleNode.Evaluate_AnyThread(Output);
    return true;
}

void FAnimSingleNodeInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext) { SingleNode.Update_AnyThread(InContext); }

void FAnimSingleNodeInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) {
    FAnimInstanceProxy::PostUpdate(InAnimInstance);
    // A montage's play state follows the instance's, which the update can change at the montage's end.
    int32 EvaluationDataIndex = 0;
    const TArray<FMontageEvaluationState>& EvaluationData = GetMontageEvaluationData();
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : InAnimInstance->MontageInstances) {
        if (MontageInstance->Montage == nullptr || MontageInstance->GetWeight() <= ZERO_ANIMWEIGHT_THRESH) continue;
        if (!EvaluationData.IsValidIndex(EvaluationDataIndex)) break;
        MontageInstance->bPlaying = EvaluationData[EvaluationDataIndex].bIsPlaying;
        EvaluationDataIndex++;
    }
}

const FMontageEvaluationState* FAnimSingleNodeInstanceProxy::GetActiveMontageEvaluationState() const {
    const TArray<FMontageEvaluationState>& EvaluationData = GetMontageEvaluationData();
    for (int32 Index = EvaluationData.Num() - 1; Index >= 0; --Index)
        if (EvaluationData[Index].bIsActive) return &EvaluationData[Index];
    return nullptr;
}

void FAnimSingleNodeInstanceProxy::UpdateMontageWeightForSlot(const FName CurrentSlotNodeName, float InGlobalNodeWeight) {
    GetSlotWeight(CurrentSlotNodeName, WeightInfo.SlotNodeWeight, WeightInfo.SourceWeight, WeightInfo.TotalNodeWeight);
    UpdateSlotNodeWeight(CurrentSlotNodeName, WeightInfo.SlotNodeWeight, InGlobalNodeWeight);
}

void FAnimSingleNodeInstanceProxy::InternalBlendSpaceEvaluatePose(const UBlendSpace* BlendSpace, FPoseContext& OutContext) {
    FAnimationPoseData AnimationPoseData = OutContext.GetAnimationPoseData();
    const FAnimExtractContext ExtractionContext(static_cast<double>(CurrentTime), ShouldExtractRootMotion(), DeltaTimeRecord, bLooping);
    if (!BlendSpace->IsValidAdditive()) {
        BlendSpace->GetAnimationPose(BlendSampleData, ExtractionContext, AnimationPoseData);
        return;
    }
    FCompactPose& OutPose = OutContext.Pose;
    FCompactPose AdditivePose;
    FBlendedCurve AdditiveCurve;
    AdditivePose.SetBoneContainer(&OutPose.GetBoneContainer());
    OutPose.ResetToRefPose();
    FAnimationPoseData AdditiveAnimationPoseData(AdditivePose, AdditiveCurve);
    BlendSpace->GetAnimationPose(BlendSampleData, ExtractionContext, AdditiveAnimationPoseData);
    const EAdditiveAnimationType AdditiveType = BlendSpace->bContainsRotationOffsetMeshSpaceSamples ? AAT_RotationOffsetMeshSpace : AAT_LocalSpaceBase;
    FAnimationRuntime::AccumulateAdditivePose(AnimationPoseData, AdditiveAnimationPoseData, 1.f, AdditiveType);
}

void FAnimSingleNodeInstanceProxy::SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping, float InPlayRate) {
    CurrentAsset = NewAsset;
    bLooping = bIsLooping;
    PlayRate = InPlayRate;
    CurrentTime = 0.f;
    BlendSpacePosition = FVector(0, 0, 0);
    BlendSampleData.Reset();
    TriangulationIndex = INDEX_NONE;
    UpdateBlendspaceSamples(BlendSpacePosition);
    if (const auto* BlendSpace = Cast<UBlendSpace>(NewAsset)) BlendSpace->InitializeFilter(&BlendFilter);
}

void FAnimSingleNodeInstanceProxy::UpdateBlendspaceSamples(const FVector& InBlendInput) {
    // UE's BlendSpaceAdvanceImmediate with no time step: the samples at the input.
    if (const auto* BlendSpace = Cast<UBlendSpace>(CurrentAsset)) BlendSpace->UpdateBlendSamples(InBlendInput, 0.f, BlendSampleData, TriangulationIndex);
}

void FAnimSingleNodeInstanceProxy::SetReverse(bool bInReverse) {
    bReverse = bInReverse;
    PlayRate = bInReverse ? -FMath::Abs(PlayRate) : FMath::Abs(PlayRate);
}

void FAnimSingleNodeInstanceProxy::GetBlendSpaceState(FVector& OutPosition, FVector& OutFilteredPosition) const {
    OutFilteredPosition = BlendFilter.GetFilterLastOutput();
    OutPosition = BlendSpacePosition;
}

float FAnimSingleNodeInstanceProxy::GetBlendSpaceLength() const {
    float TotalLength = 0.0f;
    float TotalWeight = 0.0f;
    for (const FBlendSampleData& Data : BlendSampleData) {
        const float Weight = Data.GetClampedWeight();
        TotalLength += Data.Animation->GetPlayLength() * Weight;
        TotalWeight += Weight;
    }
    return TotalWeight > 0.0f ? TotalLength / TotalWeight : 0.0f;
}

// The node.

namespace {

// A non-looping asset stops at its end in the direction it plays.
void StopAtEnd(FAnimSingleNodeInstanceProxy& Proxy, bool bLooping, float CombinedPlayRate, float CurrentTime, float PlayLength) {
    if (bLooping) return;
    if ((CombinedPlayRate < 0.f && CurrentTime <= 0.f) || (CombinedPlayRate > 0.f && CurrentTime >= PlayLength)) Proxy.SetPlaying(false);
}

}

void FAnimNode_SingleNode::Update_AnyThread(const FAnimationUpdateContext& Context) {
    const float NewPlayRate = Proxy->bPlaying ? Proxy->PlayRate : 0.f;
    UAnimationAsset* CurrentAsset = Proxy->CurrentAsset;
    if (CurrentAsset == nullptr) return;
    FAnimSync& Sync = Proxy->GetSync();
    if (auto* BlendSpace = Cast<UBlendSpace>(CurrentAsset)) {
        FAnimTickRecord TickRecord(BlendSpace, Proxy->bLooping, NewPlayRate, false, 1.f, Proxy->CurrentTime);
        TickRecord.BlendSpace.BlendSpacePositionX = static_cast<float>(Proxy->BlendSpacePosition.X);
        TickRecord.BlendSpace.BlendSpacePositionY = static_cast<float>(Proxy->BlendSpacePosition.Y);
        TickRecord.BlendSpace.BlendSampleDataCache = &Proxy->BlendSampleData;
        TickRecord.BlendSpace.BlendFilter = &Proxy->BlendFilter;
        TickRecord.BlendSpace.TriangulationIndex = &Proxy->TriangulationIndex;
        TickRecord.DeltaTimeRecord = &Proxy->DeltaTimeRecord;
        Sync.AddTickRecord(TickRecord, FAnimSyncParams());
    } else if (auto* Montage = Cast<UAnimMontage>(CurrentAsset)) {
        if (Montage->SlotAnimTracks.Num() > 0) Proxy->UpdateMontageWeightForSlot(ActiveMontageSlot, 1.f);
        if (const FMontageEvaluationState* ActiveMontageEvaluationState = Proxy->GetActiveMontageEvaluationState()) {
            Proxy->CurrentTime = ActiveMontageEvaluationState->MontagePosition;
            Proxy->DeltaTimeRecord = ActiveMontageEvaluationState->DeltaTimeRecord;
        } else if (Proxy->bPlaying) {
            Proxy->SetPlaying(false);
        }
    } else if (auto* SequenceBase = Cast<UAnimSequenceBase>(CurrentAsset)) {
        // A sequence, composite or other sequence base.
        FAnimTickRecord TickRecord(SequenceBase, Proxy->bLooping, NewPlayRate, false, 1.f, Proxy->CurrentTime);
        TickRecord.DeltaTimeRecord = &Proxy->DeltaTimeRecord;
        Sync.AddTickRecord(TickRecord, FAnimSyncParams());
        StopAtEnd(*Proxy, Proxy->bLooping, NewPlayRate * SequenceBase->RateScale, Proxy->CurrentTime, SequenceBase->GetPlayLength());
    }
}

void FAnimNode_SingleNode::Evaluate_AnyThread(FPoseContext& Output) {
    UAnimationAsset* CurrentAsset = Proxy->CurrentAsset;
    if (CurrentAsset == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    FAnimationPoseData OutputAnimationPoseData = Output.GetAnimationPoseData();
    if (const auto* BlendSpace = Cast<UBlendSpace>(CurrentAsset)) {
        Proxy->InternalBlendSpaceEvaluatePose(BlendSpace, Output);
    } else if (const auto* Sequence = Cast<UAnimSequence>(CurrentAsset)) {
        const FAnimExtractContext ExtractionContext(static_cast<double>(Proxy->CurrentTime), Sequence->bEnableRootMotion, Proxy->DeltaTimeRecord, Proxy->bLooping);
        if (!Sequence->IsValidAdditive()) {
            Sequence->GetAnimationPose(OutputAnimationPoseData, ExtractionContext);
            return;
        }
        // No editor preview: an additive sequence plays onto the reference pose.
        Output.ResetToRefPose();
        FCompactPose AdditivePose;
        FBlendedCurve AdditiveCurve;
        AdditivePose.SetBoneContainer(&Output.Pose.GetBoneContainer());
        FAnimationPoseData AdditivePoseData(AdditivePose, AdditiveCurve);
        Sequence->GetAnimationPose(AdditivePoseData, ExtractionContext);
        FAnimationRuntime::AccumulateAdditivePose(OutputAnimationPoseData, AdditivePoseData, 1.f, Sequence->AdditiveAnimType);
        Output.Pose.NormalizeRotations();
    } else if (const auto* Composite = Cast<UAnimComposite>(CurrentAsset)) {
        const FAnimExtractContext ExtractionContext(static_cast<double>(Proxy->CurrentTime), Proxy->ShouldExtractRootMotion(), Proxy->DeltaTimeRecord, Proxy->bLooping);
        const FAnimTrack& AnimTrack = Composite->AnimationTrack;
        if (!AnimTrack.IsAdditive()) {
            Composite->GetAnimationPose(OutputAnimationPoseData, ExtractionContext);
            return;
        }
        Output.Pose.ResetToRefPose();
        const EAdditiveAnimationType AdditiveAnimType = AnimTrack.IsRotationOffsetAdditive() ? AAT_RotationOffsetMeshSpace : AAT_LocalSpaceBase;
        FCompactPose AdditivePose;
        FBlendedCurve AdditiveCurve;
        AdditivePose.SetBoneContainer(&Output.Pose.GetBoneContainer());
        FAnimationPoseData AdditiveAnimationPoseData(AdditivePose, AdditiveCurve);
        Composite->GetAnimationPose(AdditiveAnimationPoseData, ExtractionContext);
        FAnimationRuntime::AccumulateAdditivePose(OutputAnimationPoseData, AdditiveAnimationPoseData, 1.f, AdditiveAnimType);
    } else if (const auto* Montage = Cast<UAnimMontage>(CurrentAsset)) {
        if (Montage->SlotAnimTracks.Num() == 0) return;
        FCompactPose LocalSourcePose;
        FBlendedCurve LocalSourceCurve;
        LocalSourcePose.SetBoneContainer(&Output.Pose.GetBoneContainer());
        LocalSourcePose.ResetToRefPose();
        const FAnimationPoseData LocalAnimationPoseData(LocalSourcePose, LocalSourceCurve);
        Proxy->SlotEvaluatePose(ActiveMontageSlot, LocalAnimationPoseData, Proxy->WeightInfo.SourceWeight, OutputAnimationPoseData,
                                Proxy->WeightInfo.SlotNodeWeight, Proxy->WeightInfo.TotalNodeWeight);
    } else {
        Output.ResetToRefPose();
    }
}

// The instance.

UAnimSingleNodeInstance::UAnimSingleNodeInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SetAnimInstanceProxy(std::make_unique<FAnimSingleNodeInstanceProxy>());
}

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAsset* NewAsset, bool bInIsLooping, float InPlayRate) {
    CurrentAsset = NewAsset;
    if (USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent()) {
        if (MeshComponent->GetSkeletalMeshAsset() == nullptr || (CurrentAsset != nullptr && CurrentAsset->GetSkeleton() == nullptr)) CurrentAsset = nullptr;
    }
    FAnimSingleNodeInstanceProxy& Proxy = GetSingleNodeProxy();
    Proxy.SetAnimationAsset(CurrentAsset, bInIsLooping, InPlayRate);
    auto* Montage = Cast<UAnimMontage>(CurrentAsset);
    if (Montage == nullptr) {
        StopAllMontages(0.25f);
        return;
    }
    Proxy.ReinitializeSlotNodes();
    if (Montage->SlotAnimTracks.Num() > 0) {
        Proxy.RegisterSlotNodeWithAnimInstance(Montage->SlotAnimTracks[0].SlotName);
        Proxy.SetMontagePreviewSlot(Montage->SlotAnimTracks[0].SlotName);
    }
    RestartMontage(Montage);
    SetPlaying(IsPlaying());
}

void UAnimSingleNodeInstance::SetMontageLoop(UAnimMontage* Montage, bool bIsLooping, FName StartingSection) {
    const int32 TotalSection = Montage->CompositeSections.Num();
    if (TotalSection <= 0) return;
    if (StartingSection.isNone()) StartingSection = Montage->CompositeSections[0].SectionName;
    const FName FirstSection = StartingSection;
    FName LastSection = StartingSection;
    bool bSucceeded = false;
    int32 CurSection = Montage->GetSectionIndex(FirstSection);
    int32 Count = TotalSection;
    // Follows the next sections from the first to the one without a next one.
    while (Count-- > 0 && CurSection != INDEX_NONE) {
        const FName NewLastSection = Montage->CompositeSections[CurSection].NextSectionName;
        CurSection = Montage->GetSectionIndex(NewLastSection);
        if (CurSection == INDEX_NONE) {
            bSucceeded = true;
            break;
        }
        Montage_SetNextSection(LastSection, NewLastSection);
        LastSection = NewLastSection;
    }
    if (bSucceeded) Montage_SetNextSection(LastSection, bIsLooping ? FirstSection : FName());
}

void UAnimSingleNodeInstance::SetMontagePreviewSlot(FName PreviewSlot) { GetSingleNodeProxy().SetMontagePreviewSlot(PreviewSlot); }

void UAnimSingleNodeInstance::UpdateMontageWeightForTimeSkip(float TimeDifference) {
    Montage_UpdateWeight(TimeDifference);
    auto* Montage = Cast<UAnimMontage>(CurrentAsset);
    if (Montage == nullptr) return;
    UpdateMontageEvaluationData();
    if (Montage->SlotAnimTracks.Num() > 0) GetSingleNodeProxy().UpdateMontageWeightForSlot(Montage->SlotAnimTracks[0].SlotName, 1.f);
}

void UAnimSingleNodeInstance::UpdateBlendspaceSamples(FVector InBlendInput) { GetSingleNodeProxy().UpdateBlendspaceSamples(InBlendInput); }

void UAnimSingleNodeInstance::RestartMontage(UAnimMontage* Montage, FName FromSection) {
    if (Montage != CurrentAsset) return;
    FAnimSingleNodeInstanceProxy& Proxy = GetSingleNodeProxy();
    Proxy.ResetWeightInfo();
    Montage_Play(Montage, Proxy.GetPlayRate());
    if (!FromSection.isNone()) Montage_JumpToSection(FromSection);
    SetMontageLoop(Montage, Proxy.IsLooping(), FromSection);
    UpdateMontageWeightForTimeSkip(Montage->BlendIn.GetBlendTime());
}

void UAnimSingleNodeInstance::NativeInitializeAnimation() {
    if (USkeletalMeshComponent* SkelComp = GetSkelMeshComponent()) SkelComp->AnimationData.Initialize(this);
}

void UAnimSingleNodeInstance::OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance) {
    if (StoppedMontageInstance.Montage == CurrentAsset) GetSingleNodeProxy().SetCurrentTime(StoppedMontageInstance.GetPosition());
    Super::OnMontageInstanceStopped(StoppedMontageInstance);
}

void UAnimSingleNodeInstance::Montage_Advance(float DeltaTime) {
    Super::Montage_Advance(DeltaTime);
    if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance()) GetSingleNodeProxy().SetCurrentTime(CurMontageInstance->GetPosition());
}

void UAnimSingleNodeInstance::PlayAnim(bool bIsLooping, float InPlayRate, float InStartPosition) {
    SetPlaying(true);
    SetLooping(bIsLooping);
    SetPlayRate(InPlayRate);
    SetPosition(InStartPosition);
}

void UAnimSingleNodeInstance::StopAnim() { SetPlaying(false); }

void UAnimSingleNodeInstance::SetLooping(bool bIsLooping) {
    FAnimSingleNodeInstanceProxy& Proxy = GetSingleNodeProxy();
    Proxy.SetLooping(bIsLooping);
    if (auto* Montage = Cast<UAnimMontage>(CurrentAsset)) SetMontageLoop(Montage, Proxy.IsLooping());
}

void UAnimSingleNodeInstance::SetPlaying(bool bIsPlaying) {
    FAnimSingleNodeInstanceProxy& Proxy = GetSingleNodeProxy();
    Proxy.SetPlaying(bIsPlaying);
    if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance()) {
        CurMontageInstance->bPlaying = bIsPlaying;
        return;
    }
    if (!Proxy.IsPlaying()) return;
    if (auto* Montage = Cast<UAnimMontage>(CurrentAsset)) RestartMontage(Montage);
}

bool UAnimSingleNodeInstance::IsPlaying() const {
    if (const FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance()) return CurMontageInstance->bPlaying;
    return GetSingleNodeProxy().IsPlaying();
}

bool UAnimSingleNodeInstance::IsReverse() const { return GetSingleNodeProxy().IsReverse(); }

bool UAnimSingleNodeInstance::IsLooping() const { return GetSingleNodeProxy().IsLooping(); }

float UAnimSingleNodeInstance::GetPlayRate() const { return GetSingleNodeProxy().GetPlayRate(); }

void UAnimSingleNodeInstance::SetPlayRate(float InPlayRate) {
    GetSingleNodeProxy().SetPlayRate(InPlayRate);
    if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance()) CurMontageInstance->SetPlayRate(InPlayRate);
}

float UAnimSingleNodeInstance::GetCurrentTime() const { return GetSingleNodeProxy().GetCurrentTime(); }

void UAnimSingleNodeInstance::SetReverse(bool bInReverse) {
    GetSingleNodeProxy().SetReverse(bInReverse);
    if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance()) CurMontageInstance->SetPlayRate(GetPlayRate());
}

void UAnimSingleNodeInstance::SetPositionWithPreviousTime(float InPosition, float InPreviousTime, bool bFireNotifies) {
    FAnimSingleNodeInstanceProxy& Proxy = GetSingleNodeProxy();
    Proxy.SetCurrentTime(FMath::Clamp(InPosition, 0.f, GetLength()));
    if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance()) CurMontageInstance->SetPosition(Proxy.GetCurrentTime());
    if (!bFireNotifies) return;
    // The notifies between the two positions fire now.
    const auto* SequenceBase = Cast<UAnimSequenceBase>(CurrentAsset);
    if (SequenceBase == nullptr) return;
    FAnimTickRecord TickRecord;
    FAnimNotifyContext NotifyContext(TickRecord);
    NotifyQueue.Reset(GetSkelMeshComponent());
    SequenceBase->GetAnimNotifiesFromDeltaPositions(InPreviousTime, Proxy.GetCurrentTime(), NotifyContext);
    if (NotifyContext.ActiveNotifies.Num() > 0) NotifyQueue.AddAnimNotifies(true, NotifyContext.ActiveNotifies, 1.0f);
    TriggerAnimNotifies(0.f);
}

void UAnimSingleNodeInstance::SetPosition(float InPosition, bool bFireNotifies) {
    SetPositionWithPreviousTime(InPosition, GetSingleNodeProxy().GetCurrentTime(), bFireNotifies);
}

void UAnimSingleNodeInstance::SetBlendSpacePosition(const FVector& InPosition) { GetSingleNodeProxy().SetBlendSpacePosition(InPosition); }

void UAnimSingleNodeInstance::GetBlendSpaceState(FVector& OutPosition, FVector& OutFilteredPosition) const {
    GetSingleNodeProxy().GetBlendSpaceState(OutPosition, OutFilteredPosition);
}

FVector UAnimSingleNodeInstance::GetFilterLastOutput() {
    return Cast<UBlendSpace>(CurrentAsset) ? GetSingleNodeProxy().GetFilterLastOutput() : FVector(0, 0, 0);
}

float UAnimSingleNodeInstance::GetLength() {
    if (Cast<UBlendSpace>(CurrentAsset)) return 1.0f;
    if (const auto* SequenceBase = Cast<UAnimSequenceBase>(CurrentAsset)) return SequenceBase->GetPlayLength();
    return 0.f;
}

}
