#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"

#include "ue5parse/runtime/Core/Math/UnrealMathUtility.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotify.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

// UE's AnimMontage.cpp (UE 5.3).

namespace ue5parse::runtime {

UAnimMontage::UAnimMontage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SlotAnimTracks.AddDefaulted();
    SlotAnimTracks[0].SlotName = FAnimSlotGroup::DefaultSlotName;
}

UAnimMontage* UAnimMontage::CreateSlotAnimationAsDynamicMontage_WithBlendSettings(UAnimSequenceBase* Asset, FName SlotNodeName,
                                                                                 const FMontageBlendSettings& BlendInSettings,
                                                                                 const FMontageBlendSettings& BlendOutSettings, float InPlayRate,
                                                                                 int32 LoopCount, float InBlendOutTriggerTime) {
    if (Asset == nullptr || Asset->IsA(UAnimMontage::StaticClass()) || SlotNodeName.isNone()) return nullptr;
    auto* NewMontage = NewObject<UAnimMontage>(GetTransientPackage());
    NewMontage->Skeleton = Asset->GetSkeleton();
    FSlotAnimationTrack& NewTrack = NewMontage->SlotAnimTracks[0];
    NewTrack.SlotName = SlotNodeName;
    FAnimSegment NewSegment;
    NewSegment.AnimReference = Asset;
    NewSegment.AnimEndTime = Asset->GetPlayLength();
    NewSegment.LoopingCount = LoopCount;
    NewMontage->SequenceLength = NewSegment.GetLength();
    NewTrack.AnimTrack.AnimSegments.Add(NewSegment);

    FCompositeSection NewSection;
    NewSection.SectionName = FName("Default");
    NewSection.LinkedSequence = Asset;
    NewMontage->CompositeSections.Add(NewSection);

    NewMontage->BlendIn = FAlphaBlend(BlendInSettings.Blend);
    NewMontage->BlendModeIn = BlendInSettings.BlendMode;
    NewMontage->BlendProfileIn = BlendInSettings.BlendProfile;
    NewMontage->BlendOut = FAlphaBlend(BlendOutSettings.Blend);
    NewMontage->BlendModeOut = BlendOutSettings.BlendMode;
    NewMontage->BlendProfileOut = BlendOutSettings.BlendProfile;
    NewMontage->BlendOutTriggerTime = InBlendOutTriggerTime;
    return NewMontage;
}

int32 UAnimMontage::GetSectionIndex(FName InSectionName) const {
    for (int32 i = 0; i < CompositeSections.Num(); i++)
        if (CompositeSections[i].SectionName == InSectionName) return i;
    return INDEX_NONE;
}

FName UAnimMontage::GetSectionName(int32 SectionIndex) const {
    return CompositeSections.IsValidIndex(SectionIndex) ? CompositeSections[SectionIndex].SectionName : FName();
}

bool UAnimMontage::IsWithinPos(int32 FirstIndex, int32 SecondIndex, float CurrentTime) const {
    const float StartTime = CompositeSections.IsValidIndex(FirstIndex) ? CompositeSections[FirstIndex].GetTime() : 0.0f;
    const float EndTime = CompositeSections.IsValidIndex(SecondIndex) ? CompositeSections[SecondIndex].GetTime() : GetPlayLength();
    // The sections are [start, end), but the last one also contains the end of the montage.
    if (CurrentTime >= GetPlayLength()) return StartTime <= CurrentTime && EndTime >= CurrentTime;
    return StartTime <= CurrentTime && EndTime > CurrentTime;
}

int32 UAnimMontage::GetAnimCompositeSectionIndexFromPos(float CurrentTime, float& PosWithinCompositeSection) const {
    PosWithinCompositeSection = 0.0f;
    for (int32 I = 0; I < CompositeSections.Num(); ++I) {
        if (!IsWithinPos(I, I + 1, CurrentTime)) continue;
        PosWithinCompositeSection = CurrentTime - CompositeSections[I].GetTime();
        return I;
    }
    return INDEX_NONE;
}

void UAnimMontage::GetSectionStartAndEndTime(int32 SectionIndex, float& OutStartTime, float& OutEndTime) const {
    OutStartTime = IsValidSectionIndex(SectionIndex) ? CompositeSections[SectionIndex].GetTime() : 0.0f;
    OutEndTime = IsValidSectionIndex(SectionIndex + 1) ? CompositeSections[SectionIndex + 1].GetTime() : GetPlayLength();
}

float UAnimMontage::GetSectionLength(int32 SectionIndex) const {
    float StartTime, EndTime;
    GetSectionStartAndEndTime(SectionIndex, StartTime, EndTime);
    return EndTime - StartTime;
}

bool UAnimMontage::IsValidSlot(FName InSlotName) const {
    const FAnimTrack* Track = GetAnimationData(InSlotName);
    return Track != nullptr && Track->AnimSegments.Num() > 0;
}

bool UAnimMontage::IsValidAdditiveSlot(FName SlotNodeName) const {
    const FAnimTrack* Track = GetAnimationData(SlotNodeName);
    return Track != nullptr && Track->IsAdditive();
}

const FAnimTrack* UAnimMontage::GetAnimationData(FName InSlotName) const {
    for (const FSlotAnimationTrack& Track : SlotAnimTracks)
        if (Track.SlotName == InSlotName) return &Track.AnimTrack;
    return nullptr;
}

bool UAnimMontage::HasValidSlotSetup() const {
    if (SlotAnimTracks.Num() <= 1 || Skeleton == nullptr) return true;
    const FName MontageGroupName = GetGroupName();
    for (int32 TrackIndex = 1; TrackIndex < SlotAnimTracks.Num(); TrackIndex++) {
        const FName CurrentSlotName = SlotAnimTracks[TrackIndex].SlotName;
        for (int32 Other = 0; Other < TrackIndex; Other++)
            if (SlotAnimTracks[Other].SlotName == CurrentSlotName) return false;
        if (Skeleton->GetSlotGroupName(CurrentSlotName) != MontageGroupName) return false;
    }
    return true;
}

bool UAnimMontage::HasRootMotion() const {
    for (const FSlotAnimationTrack& Track : SlotAnimTracks)
        for (const FAnimSegment& Segment : Track.AnimTrack.AnimSegments)
            if (Segment.GetAnimReference() != nullptr && Segment.GetAnimReference()->HasRootMotion()) return true;
    return false;
}

const FBranchingPointMarker* UAnimMontage::FindFirstBranchingPointMarker(float StartTrackPos, float EndTrackPos) const {
    if (EndTrackPos >= StartTrackPos) {
        for (const FBranchingPointMarker& Marker : BranchingPointMarkers) {
            if (Marker.TriggerTime <= StartTrackPos) continue;
            if (Marker.TriggerTime > EndTrackPos) break;
            return &Marker;
        }
        return nullptr;
    }
    for (int32 Index = BranchingPointMarkers.Num() - 1; Index >= 0; Index--) {
        const FBranchingPointMarker& Marker = BranchingPointMarkers[Index];
        if (Marker.TriggerTime >= StartTrackPos) continue;
        if (Marker.TriggerTime < EndTrackPos) break;
        return &Marker;
    }
    return nullptr;
}

FName UAnimMontage::GetGroupName() const {
    // The group of the first slot.
    if (Skeleton != nullptr && SlotAnimTracks.Num() > 0) return Skeleton->GetSlotGroupName(SlotAnimTracks[0].SlotName);
    return FAnimSlotGroup::DefaultGroupName;
}

void FMontageSubStepper::Initialize(const FAnimMontageInstance& InAnimInstance) {
    MontageInstance = &InAnimInstance;
    Montage = MontageInstance->Montage;
}

EMontageSubStepResult FMontageSubStepper::Advance(float& InOut_P_Original, const FBranchingPointMarker** OutBranchingPointMarkerPtr) {
    DeltaMove = 0.0f;
    if (MontageInstance == nullptr || Montage == nullptr) return EMontageSubStepResult::InvalidMontage;
    bReachedEndOfSection = false;
    float PositionInSection;
    CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(InOut_P_Original, PositionInSection);
    if (!Montage->IsValidSectionIndex(CurrentSectionIndex)) return EMontageSubStepResult::InvalidSection;
    const FCompositeSection& CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);
    CurrentSectionStartTime = CurrentSection.GetTime();
    CurrentSectionLength = Montage->GetSectionLength(CurrentSectionIndex);
    if (!MontageInstance->bPlaying || FMath::IsNearlyZero(TimeRemaining)) return EMontageSubStepResult::NotMoved;

    PlayRate = MontageInstance->PlayRate * Montage->RateScale;
    if (FMath::IsNearlyZero(PlayRate)) return EMontageSubStepResult::NotMoved;
    bPlayingForward = PlayRate > 0.0f;
    DeltaMove = TimeRemaining * PlayRate;

    if (OutBranchingPointMarkerPtr != nullptr) {
        *OutBranchingPointMarkerPtr = Montage->FindFirstBranchingPointMarker(InOut_P_Original, InOut_P_Original + DeltaMove);
        if (*OutBranchingPointMarkerPtr != nullptr) DeltaMove = (*OutBranchingPointMarkerPtr)->TriggerTime - InOut_P_Original;
    }

    // The step ends at the section's end; a branching point behind that end is not reached.
    const float OldDeltaMove = DeltaMove;
    if (bPlayingForward) {
        const float MaxSectionMove = CurrentSectionLength - PositionInSection;
        if (DeltaMove >= MaxSectionMove) {
            DeltaMove = MaxSectionMove;
            bReachedEndOfSection = true;
        }
    } else {
        const float MinSectionMove = -PositionInSection;
        if (DeltaMove <= MinSectionMove) {
            DeltaMove = MinSectionMove;
            bReachedEndOfSection = true;
        }
    }
    if (OutBranchingPointMarkerPtr != nullptr && *OutBranchingPointMarkerPtr != nullptr && OldDeltaMove != DeltaMove)
        *OutBranchingPointMarkerPtr = nullptr;

    if (FMath::Abs(DeltaMove) <= 0.0f) return EMontageSubStepResult::NotMoved;
    InOut_P_Original += DeltaMove;
    TimeRemaining = FMath::Max(TimeRemaining - DeltaMove / PlayRate, 0.0f);
    return EMontageSubStepResult::Moved;
}

float FMontageSubStepper::GetRemainingPlayTimeToSectionEnd(float In_P_Original) const {
    if (FMath::IsNearlyZero(PlayRate)) return UE_BIG_NUMBER;
    const float CurrSectionEnd_Original = bPlayingForward ? CurrentSectionStartTime + CurrentSectionLength : CurrentSectionStartTime;
    return FMath::Abs((CurrSectionEnd_Original - In_P_Original) / PlayRate);
}

void FAnimMontageInstance::Initialize(UAnimMontage* InMontage) {
    static int32 IncrementInstanceID = 0;
    InstanceID = IncrementInstanceID++;
    if (InMontage == nullptr) return;
    Montage = InMontage;
    SetPosition(0.0f);
    BlendStartAlpha = 0.0f;
    Blend.SetValueRange(0.0f, 1.0f);
    RefreshNextPrevSections();
    MontageSubStepper.Initialize(*this);
}

void FAnimMontageInstance::Play(float InPlayRate) {
    FMontageBlendSettings BlendInSettings;
    BlendInSettings.Blend = FAlphaBlendArgs(Montage->BlendIn);
    BlendInSettings.BlendMode = Montage->BlendModeIn;
    BlendInSettings.BlendProfile = Montage->BlendProfileIn;
    Play(InPlayRate, BlendInSettings);
}

void FAnimMontageInstance::Play(float InPlayRate, const FMontageBlendSettings& BlendInSettings) {
    bPlaying = true;
    PlayRate = InPlayRate;
    FAlphaBlendArgs BlendInArgs = BlendInSettings.Blend;
    // An inertial blend pops in: the inertialization node that would smooth it is not implemented yet.
    if (BlendInSettings.BlendMode == EMontageBlendMode::Inertialization) BlendInArgs.BlendTime = 0.0f;
    const float CurrentWeight = Blend.GetBlendedValue();
    InitializeBlend(FAlphaBlend(BlendInArgs));
    BlendStartAlpha = Blend.GetAlpha();
    Blend.SetBlendTime(BlendInArgs.BlendTime * DefaultBlendTimeMultiplier);
    Blend.SetValueRange(CurrentWeight, 1.0f);
    bEnableAutoBlendOut = Montage->bEnableAutoBlendOut;
    ActiveBlendProfile = BlendInSettings.BlendProfile;
}

void FAnimMontageInstance::InitializeBlend(const FAlphaBlend& InAlphaBlend) {
    Blend.SetBlendOption(InAlphaBlend.GetBlendOption());
    Blend.SetCustomCurve(InAlphaBlend.GetCustomCurve());
    Blend.SetBlendTime(InAlphaBlend.GetBlendTime());
}

void FAnimMontageInstance::Stop(const FMontageBlendSettings& InBlendOutSettings, bool bInterrupt) {
    if (!bInterrupted && bInterrupt) bInterrupted = bInterrupt;
    if (!IsStopped()) {
        FAlphaBlendArgs BlendOutArgs = InBlendOutSettings.Blend;
        if (InBlendOutSettings.BlendMode == EMontageBlendMode::Inertialization) BlendOutArgs.BlendTime = 0.0f;
        InitializeBlend(FAlphaBlend(BlendOutArgs));
        BlendStartAlpha = Blend.GetAlpha();
        Blend.SetDesiredValue(0.0f);
        Blend.Update(0.0f);
        ActiveBlendProfile = InBlendOutSettings.BlendProfile;
        if (Montage != nullptr && AnimInstance != nullptr) {
            AnimInstance->OnMontageInstanceStopped(*this);
            AnimInstance->QueueMontageBlendingOutEvent(FQueuedMontageBlendingOutEvent{Montage, bInterrupted, OnMontageBlendingOutStarted});
        }
    } else if (InBlendOutSettings.Blend.BlendTime < Blend.GetBlendTime()) {
        // Already blending out: a shorter blend out replaces the current one.
        Blend.SetBlendTime(InBlendOutSettings.Blend.BlendTime);
        BlendStartAlpha = Blend.GetAlpha();
        Blend.SetDesiredValue(0.0f);
    }
    if (Blend.GetBlendTime() <= 0.0f) bPlaying = false;
}

void FAnimMontageInstance::Stop(const FAlphaBlend& InBlendOut, bool bInterrupt) {
    FMontageBlendSettings BlendOutSettings;
    BlendOutSettings.Blend = FAlphaBlendArgs(InBlendOut);
    if (Montage != nullptr) {
        BlendOutSettings.BlendMode = Montage->BlendModeOut;
        BlendOutSettings.BlendProfile = Montage->BlendProfileOut;
    }
    Stop(BlendOutSettings, bInterrupt);
}

void FAnimMontageInstance::RefreshNextPrevSections() {
    const int32 NumSections = Montage->CompositeSections.Num();
    if (NumSections == 0) return;
    NextSections.Empty();
    NextSections.SetNum(NumSections);
    PrevSections.Empty();
    PrevSections.SetNum(NumSections);
    for (int32 I = 0; I < NumSections; ++I) PrevSections[I] = INDEX_NONE;
    for (int32 I = 0; I < NumSections; ++I) {
        const int32 NextSectionIdx = Montage->GetSectionIndex(Montage->CompositeSections[I].NextSectionName);
        NextSections[I] = NextSectionIdx;
        if (NextSections.IsValidIndex(NextSectionIdx)) PrevSections[NextSectionIdx] = I;
    }
}

void FAnimMontageInstance::Terminate() {
    if (Montage == nullptr) return;
    UAnimMontage* OldMontage = Montage;
    if (AnimInstance != nullptr) {
        // The branching point states end, without reaching their end.
        for (int32 Index = ActiveStateBranchingPoints.Num() - 1; Index >= 0; Index--) {
            FAnimNotifyEvent& NotifyEvent = ActiveStateBranchingPoints[Index];
            if (NotifyEvent.NotifyStateClass == nullptr) continue;
            FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID, false);
            NotifyEvent.NotifyStateClass->BranchingPointNotifyEnd(BranchingPointNotifyPayload);
            if (!ValidateInstanceAfterNotifyState(AnimInstance)) return;
        }
        ActiveStateBranchingPoints.Empty();
        AnimInstance->QueueMontageEndedEvent(FQueuedMontageEndedEvent{OldMontage, InstanceID, bInterrupted, OnMontageEnded});
        AnimInstance->ClearMontageInstanceReferences(*this);
    }
    Blend.SetCustomCurve(nullptr);
    Blend.SetBlendOption(EAlphaBlendOption::Linear);
    ActiveBlendProfile = nullptr;
    Montage = nullptr;
}

bool FAnimMontageInstance::JumpToSectionName(FName SectionName, bool bEndOfSection) {
    const int32 SectionID = Montage->GetSectionIndex(SectionName);
    if (!Montage->IsValidSectionIndex(SectionID)) return false;
    const FCompositeSection& CurSection = Montage->GetAnimCompositeSection(SectionID);
    SetPosition(Montage->CalculatePos(CurSection, bEndOfSection ? Montage->GetSectionLength(SectionID) - UE_KINDA_SMALL_NUMBER : 0.0f));
    OnMontagePositionChanged(SectionName);
    return true;
}

bool FAnimMontageInstance::SetNextSectionName(FName SectionName, FName NewNextSectionName) {
    return SetNextSectionID(Montage->GetSectionIndex(SectionName), Montage->GetSectionIndex(NewNextSectionName));
}

bool FAnimMontageInstance::SetNextSectionID(int32 SectionID, int32 NewNextSectionID) {
    const bool bHasValidNextSection = NextSections.IsValidIndex(SectionID);
    // The old next section loses this section as its previous one.
    if (bHasValidNextSection && NextSections[SectionID] != INDEX_NONE && PrevSections.IsValidIndex(NextSections[SectionID]))
        PrevSections[NextSections[SectionID]] = INDEX_NONE;
    if (PrevSections.IsValidIndex(NewNextSectionID)) PrevSections[NewNextSectionID] = SectionID;
    if (!bHasValidNextSection) return false;
    NextSections[SectionID] = NewNextSectionID;
    OnMontagePositionChanged(GetSectionNameFromID(NewNextSectionID));
    return true;
}

void FAnimMontageInstance::OnMontagePositionChanged(FName ToSectionName) {
    // A section change during the blend out plays the montage again.
    if (bPlaying && IsStopped()) Play(PlayRate);
}

FName FAnimMontageInstance::GetCurrentSection() const {
    if (Montage == nullptr) return FName();
    float CurrentPosition;
    const int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(Position, CurrentPosition);
    return Montage->IsValidSectionIndex(CurrentSectionIndex) ? Montage->GetAnimCompositeSection(CurrentSectionIndex).SectionName : FName();
}

FName FAnimMontageInstance::GetNextSection() const {
    if (Montage == nullptr) return FName();
    float CurrentPosition;
    const int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(Position, CurrentPosition);
    return Montage->IsValidSectionIndex(CurrentSectionIndex) ? Montage->GetAnimCompositeSection(CurrentSectionIndex).NextSectionName : FName();
}

FName FAnimMontageInstance::GetSectionNameFromID(int32 SectionID) const {
    return Montage != nullptr && Montage->IsValidSectionIndex(SectionID) ? Montage->GetAnimCompositeSection(SectionID).SectionName : FName();
}

void FAnimMontageInstance::UpdateWeight(float DeltaTime) {
    if (!IsValid()) return;
    PreviousWeight = Blend.GetBlendedValue();
    Blend.Update(DeltaTime);
    if (Blend.GetBlendTimeRemaining() < 0.0001f) ActiveBlendProfile = nullptr;
    // A notify of this frame can come from any time since the last frame.
    NotifyWeight = FMath::Max(PreviousWeight, Blend.GetBlendedValue());
}

void FAnimMontageInstance::Advance(float DeltaTime) {
    if (IsValid() && bPlaying) {
        DeltaTimeRecord.Set(Position, 0.0f);
        // A high play rate or a long frame on a looping montage stops after a few steps.
        const int32 MaxIterations = 10;
        int32 NumIterations = 0;
        if (MontageSubStepper.GetRemainingTime() < 10.0f) MontageSubStepper.AddEvaluationTime(DeltaTime);
        // A paused frame still fires the notifies at the position.
        if (DeltaTime == 0.f) HandleEvents(Position, Position, nullptr);
        while (bPlaying && MontageSubStepper.HasTimeRemaining() && ++NumIterations < MaxIterations) {
            const FBranchingPointMarker* BranchingPointMarker = nullptr;
            const float PreviousSubStepPosition = Position;
            EMontageSubStepResult SubStepResult = MontageSubStepper.Advance(Position, &BranchingPointMarker);
            if (SubStepResult == EMontageSubStepResult::InvalidSection || SubStepResult == EMontageSubStepResult::InvalidMontage) {
                Stop(FAlphaBlend(Montage->BlendOut, Montage->BlendOut.GetBlendTime() * DefaultBlendTimeMultiplier), false);
                break;
            }
            DeltaTimeRecord.Delta += MontageSubStepper.GetDeltaMove();
            const bool bPlayingForward = MontageSubStepper.GetbPlayingForward();

            // The auto blend out starts when the end of the section chain is closer than the blend out.
            if (!IsStopped() && bEnableAutoBlendOut) {
                const int32 CurrentSectionIndex = MontageSubStepper.GetCurrentSectionIndex();
                const int32 NextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];
                if (NextSectionIndex == INDEX_NONE) {
                    const float PlayTimeToEnd = MontageSubStepper.GetRemainingPlayTimeToSectionEnd(Position);
                    const bool bCustomBlendOutTriggerTime = Montage->BlendOutTriggerTime >= 0;
                    const float DefaultBlendOutTime = Montage->BlendOut.GetBlendTime() * DefaultBlendTimeMultiplier;
                    const float BlendOutTriggerTime = bCustomBlendOutTriggerTime ? Montage->BlendOutTriggerTime : DefaultBlendOutTime;
                    if (PlayTimeToEnd <= FMath::Max(BlendOutTriggerTime, UE_KINDA_SMALL_NUMBER))
                        Stop(FAlphaBlend(Montage->BlendOut, bCustomBlendOutTriggerTime ? DefaultBlendOutTime : PlayTimeToEnd), false);
                }
            }

            const float PositionBeforeFiringEvents = Position;
            if (SubStepResult == EMontageSubStepResult::Moved && !bInterrupted) {
                UAnimInstance* AnimInstanceLocal = AnimInstance;
                HandleEvents(PreviousSubStepPosition, Position, BranchingPointMarker);
                // An event can stop every montage, which deletes this instance.
                if (AnimInstanceLocal != nullptr && AnimInstanceLocal->MontageInstances.Num() == 0) return;
            }

            // The section chain continues at the next section's start, or its end when playing backwards. A branching
            // point that moved the position (a jump) wins.
            if (MontageSubStepper.HasReachedEndOfSection() && BranchingPointMarker == nullptr && PositionBeforeFiringEvents == Position) {
                const int32 CurrentSectionIndex = MontageSubStepper.GetCurrentSectionIndex();
                const int32 RecentNextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];
                if (RecentNextSectionIndex == INDEX_NONE) break;
                float LatestNextSectionStartTime, LatestNextSectionEndTime;
                Montage->GetSectionStartAndEndTime(RecentNextSectionIndex, LatestNextSectionStartTime, LatestNextSectionEndTime);
                Position = bPlayingForward ? LatestNextSectionStartTime : LatestNextSectionEndTime - UE_KINDA_SMALL_NUMBER / 2.0f;
                SubStepResult = EMontageSubStepResult::Moved;
            }
            if (SubStepResult == EMontageSubStepResult::NotMoved) break;
        }
    }
    if (IsStopped() && Blend.IsComplete()) {
        Terminate();
        return;
    }
    if (bInterrupted || AnimInstance == nullptr) return;
    for (int32 Index = 0; Index < ActiveStateBranchingPoints.Num(); Index++) {
        FAnimNotifyEvent& NotifyEvent = ActiveStateBranchingPoints[Index];
        if (NotifyEvent.NotifyStateClass == nullptr) continue;
        FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID);
        NotifyEvent.NotifyStateClass->BranchingPointNotifyTick(BranchingPointNotifyPayload, DeltaTime);
        if (!ValidateInstanceAfterNotifyState(AnimInstance)) return;
    }
}

void UAnimMontage::FilterOutNotifyBranchingPoints(TArray<FAnimNotifyEventReference>& InAnimNotifies) {
    for (int32 Index = InAnimNotifies.Num() - 1; Index >= 0; Index--) {
        const FAnimNotifyEvent* Notify = InAnimNotifies[Index].GetNotify();
        if (Notify == nullptr || Notify->IsBranchingPoint()) InAnimNotifies.RemoveAt(Index, 1);
    }
}

bool FAnimMontageInstance::ValidateInstanceAfterNotifyState(UAnimInstance* InAnimInstance) const {
    if (InAnimInstance == nullptr) return false;
    for (const std::unique_ptr<FAnimMontageInstance>& Instance : InAnimInstance->MontageInstances)
        if (Instance.get() == this) return true;
    return false;
}

void FAnimMontageInstance::HandleEvents(float PreviousTrackPos, float CurrentTrackPos, const FBranchingPointMarker* BranchingPointMarker) {
    if (bInterrupted) return;
    if (AnimInstance != nullptr) {
        // The montage's own notifies, without the branching points.
        FAnimTickRecord TickRecord;
        FAnimNotifyContext NotifyContext(TickRecord);
        Montage->GetAnimNotifiesFromDeltaPositions(PreviousTrackPos, CurrentTrackPos, NotifyContext);
        UAnimMontage::FilterOutNotifyBranchingPoints(NotifyContext.ActiveNotifies);
        AnimInstance->NotifyQueue.AddAnimNotifies(NotifyContext.ActiveNotifies, NotifyWeight);

        // The notifies of each slot's animations, which count only when the slot is relevant.
        TMap<FName, TArray<FAnimNotifyEventReference>> NotifyMap;
        for (const FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks) {
            TArray<FAnimNotifyEventReference>& CurrentSlotNotifies = NotifyMap.FindOrAdd(SlotTrack.SlotName);
            NotifyContext.ActiveNotifies.Reset();
            SlotTrack.AnimTrack.GetAnimNotifiesFromTrackPositions(PreviousTrackPos, CurrentTrackPos, NotifyContext);
            CurrentSlotNotifies = NotifyContext.ActiveNotifies;
        }
        AnimInstance->NotifyQueue.AddAnimNotifies(NotifyMap, NotifyWeight);
    }
    if (!UpdateActiveStateBranchingPoints(CurrentTrackPos)) return;
    if (BranchingPointMarker) BranchingPointEventHandler(BranchingPointMarker);
}

void FAnimMontageInstance::BranchingPointEventHandler(const FBranchingPointMarker* BranchingPointMarker) {
    if (AnimInstance == nullptr || Montage == nullptr || BranchingPointMarker == nullptr) return;
    UAnimInstance* AnimInstanceLocal = AnimInstance;
    FAnimNotifyEvent* NotifyEvent = BranchingPointMarker->NotifyIndex < Montage->Notifies.Num() ? &Montage->Notifies[BranchingPointMarker->NotifyIndex] : nullptr;
    if (NotifyEvent == nullptr) return;
    if (NotifyEvent->bConvertedFromBranchingPoint && !NotifyEvent->NotifyName.isNone()) {
        // An old branching point calls MontageBranchingPoint_<Name>, else the notify's event.
        const FName FuncFName(std::string("MontageBranchingPoint_") + NotifyEvent->NotifyName.toString());
        if (UFunction* Function = AnimInstance->FindFunction(FuncFName)) AnimInstance->ProcessEvent(Function, nullptr);
        else AnimInstance->TriggerSingleAnimNotify(NotifyEvent);
    } else if (NotifyEvent->NotifyStateClass != nullptr) {
        if (BranchingPointMarker->NotifyEventType == EAnimNotifyEventType::Begin) {
            FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, NotifyEvent, InstanceID);
            NotifyEvent->NotifyStateClass->BranchingPointNotifyBegin(BranchingPointNotifyPayload);
            if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal)) return;
            ActiveStateBranchingPoints.Add(*NotifyEvent);
        } else {
            FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, NotifyEvent, InstanceID, true);
            NotifyEvent->NotifyStateClass->BranchingPointNotifyEnd(BranchingPointNotifyPayload);
            if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal)) return;
            ActiveStateBranchingPoints.RemoveSingleSwap(*NotifyEvent);
        }
    } else if (NotifyEvent->Notify != nullptr) {
        FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, NotifyEvent, InstanceID);
        NotifyEvent->Notify->BranchingPointNotify(BranchingPointNotifyPayload);
    } else {
        AnimInstance->TriggerSingleAnimNotify(NotifyEvent);
    }
}

bool FAnimMontageInstance::UpdateActiveStateBranchingPoints(float CurrentTrackPosition) {
    const int32 NumStateBranchingPoints = Montage->BranchingPointStateNotifyIndices.Num();
    if (AnimInstance == nullptr || NumStateBranchingPoints == 0) return true;
    UAnimInstance* AnimInstanceLocal = AnimInstance;
    // The states that the position left end.
    for (int32 Index = ActiveStateBranchingPoints.Num() - 1; Index >= 0; Index--) {
        FAnimNotifyEvent& NotifyEvent = ActiveStateBranchingPoints[Index];
        if (NotifyEvent.NotifyStateClass == nullptr) continue;
        const bool bNotifyIsActive = CurrentTrackPosition > NotifyEvent.GetTriggerTime() && CurrentTrackPosition <= NotifyEvent.GetEndTriggerTime();
        if (bNotifyIsActive) continue;
        FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID, true);
        NotifyEvent.NotifyStateClass->BranchingPointNotifyEnd(BranchingPointNotifyPayload);
        if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal)) return false;
        ActiveStateBranchingPoints.RemoveAt(Index, 1);
    }
    // The states that the position entered begin.
    for (int32 Index = 0; Index < NumStateBranchingPoints; Index++) {
        const int32 NotifyIndex = Montage->BranchingPointStateNotifyIndices[Index];
        FAnimNotifyEvent& NotifyEvent = Montage->Notifies[NotifyIndex];
        if (NotifyEvent.NotifyStateClass == nullptr) continue;
        const bool bNotifyIsActive = CurrentTrackPosition > NotifyEvent.GetTriggerTime() && CurrentTrackPosition <= NotifyEvent.GetEndTriggerTime();
        if (!bNotifyIsActive || ActiveStateBranchingPoints.Contains(NotifyEvent)) continue;
        FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID);
        NotifyEvent.NotifyStateClass->BranchingPointNotifyBegin(BranchingPointNotifyPayload);
        if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal)) return false;
        ActiveStateBranchingPoints.Add(NotifyEvent);
    }
    return true;
}

}
