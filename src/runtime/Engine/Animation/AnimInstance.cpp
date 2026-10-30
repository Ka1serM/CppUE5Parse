#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotify.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"

namespace ue5parse::runtime {

UAnimInstance::UAnimInstance(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), AnimInstanceProxy(std::make_unique<FAnimInstanceProxy>()) {}

UAnimInstance::~UAnimInstance() = default;

void UAnimInstance::SetAnimInstanceProxy(std::unique_ptr<FAnimInstanceProxy> InProxy) { AnimInstanceProxy = std::move(InProxy); }

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponent() const { return Cast<USkeletalMeshComponent>(GetOuter()); }

bool UAnimInstance::GetCurveValueWithDefault(FName CurveName, float DefaultValue, float& OutValue) const {
    bool bHasValue = false;
    OutValue = AnimationCurves.Get(CurveName, bHasValue, DefaultValue);
    return bHasValue;
}

float UAnimInstance::GetInstanceAssetPlayerTimeFromEndFraction(int32 AssetPlayerIndex) const {
    // The runtime keeps the state machines' queries; the asset-player list queries are not used by the cooked graphs.
    return 0.0f;
}

AActor* UAnimInstance::GetOwningActor() const {
    USkeletalMeshComponent* Component = GetSkelMeshComponent();
    return Component != nullptr ? Component->GetOwner() : nullptr;
}

USkeletalMeshComponent* UAnimInstance::GetOwningComponent() const { return GetSkelMeshComponent(); }

void UAnimInstance::ProcessCopies(int32 CallSite) {
    const auto* AnimClass = Cast<UAnimBlueprintGeneratedClass>(GetClass());
    if (AnimClass == nullptr || AnimClass->GetPropertyAccessLibrary() == nullptr) return;
    AnimClass->GetPropertyAccessLibrary()->ProcessCopies(this, CallSite);
}

void UAnimInstance::UninitializeAnimation() {
    NativeUninitializeAnimation();
    StopAllMontages(0.f);
    if (MontageInstances.Num() > 0) {
        for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances) ClearMontageInstanceReferences(*MontageInstance);
        MontageInstances.Empty();
        ActiveMontagesMap.Empty();
        OnAllMontageInstancesEnded.ProcessMulticastDelegate(nullptr);
    }
    if (GetSkelMeshComponent() != nullptr) {
        // The active states end.
        USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();
        for (int32 Index = 0; Index < ActiveAnimNotifyState.Num(); Index++) {
            const FAnimNotifyEvent& AnimNotifyEvent = ActiveAnimNotifyState[Index];
            const FAnimNotifyEventReference& EventReference = ActiveAnimNotifyEventReference[Index];
            if (!ShouldTriggerAnimNotifyState(AnimNotifyEvent.NotifyStateClass)) continue;
            AnimNotifyEvent.NotifyStateClass->NotifyEnd(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent.NotifyStateClass->GetOuter()), EventReference);
        }
    }
    ActiveAnimNotifyState.Reset();
    ActiveAnimNotifyEventReference.Reset();
    NotifyQueue.Reset(GetSkelMeshComponent());
}

void UAnimInstance::InitializeAnimation(bool bInDeferRootNodeInitialization) {
    UninitializeAnimation();
    USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
    const USkeletalMesh* Mesh = OwnerComponent->GetSkeletalMeshAsset();
    CurrentSkeleton = Mesh ? Mesh->GetSkeleton() : nullptr;
    FAnimInstanceProxy& Proxy = GetProxyOnGameThread();
    if (Mesh != nullptr) Proxy.RecalcRequiredBones(OwnerComponent, const_cast<USkeletalMesh*>(Mesh));
    Proxy.Initialize(this);
    NativeInitializeAnimation();
    BlueprintInitializeAnimation();
    Proxy.InitializeRootNode(bInDeferRootNodeInitialization);
}

FAnimInstanceProxy& UAnimInstance::GetProxyOnGameThread() {
    if (USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(GetOuter())) Component->HandleExistingParallelEvaluationTask(true, true);
    return *AnimInstanceProxy;
}

const FAnimInstanceProxy& UAnimInstance::GetProxyOnGameThread() const { return const_cast<UAnimInstance*>(this)->GetProxyOnGameThread(); }

void UAnimInstance::UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion, EUpdateAnimationFlag UpdateFlag) {
    // UE's PreUpdateAnimation.
    bNeedsUpdate = true;
    NotifyQueue.Reset(GetSkelMeshComponent());
    GetProxyOnGameThread().PreUpdate(this, DeltaSeconds);
    // The montages advance before the graph, so that its slots know where they are.
    UpdateMontage(DeltaSeconds);
    UpdateMontageEvaluationData();
    ProcessCopies(EAnimPropertyAccessCallSite::GameThread_Batched_PreEventGraph);
    NativeUpdateAnimation(DeltaSeconds);
    BlueprintUpdateAnimation(DeltaSeconds);
    ProcessCopies(EAnimPropertyAccessCallSite::GameThread_Batched_PostEventGraph);
    if (UpdateFlag != EUpdateAnimationFlag::ForceParallelUpdate && NeedsImmediateUpdate(DeltaSeconds, bNeedsValidRootMotion)) {
        ParallelUpdateAnimation();
        PostUpdateAnimation();
    }
}

bool UAnimInstance::NeedsImmediateUpdate(float DeltaSeconds, bool bNeedsValidRootMotion) const {
    return (bNeedsValidRootMotion && RootMotionMode == ERootMotionMode::RootMotionFromEverything) || !CanRunParallelWork() ||
           !bUseMultiThreadedAnimationUpdate || DeltaSeconds == 0.0f;
}

void UAnimInstance::ParallelUpdateAnimation() { AnimInstanceProxy->UpdateAnimation(); }

void UAnimInstance::PostUpdateAnimation() {
    // The main instance post updates its linked instances, whose notifies its graph update queued.
    USkeletalMeshComponent* Component = GetSkelMeshComponent();
    if (Component->GetAnimInstance() == this)
        for (UAnimInstance* LinkedInstance : Component->GetLinkedAnimInstances())
            if (LinkedInstance->NeedsUpdate()) LinkedInstance->PostUpdateAnimation();
    if (!bNeedsUpdate) return;
    bNeedsUpdate = false;
    GetProxyOnGameThread().PostUpdate(this);
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds) {
    USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();
    TArray<FAnimNotifyEvent> NewActiveAnimNotifyState;
    NewActiveAnimNotifyState.Reserve(NotifyQueue.AnimNotifies.Num());
    TArray<FAnimNotifyEventReference> NewActiveAnimNotifyEventReference;
    NewActiveAnimNotifyEventReference.Reserve(NotifyQueue.AnimNotifies.Num());
    // The states that begin this frame.
    TArray<const FAnimNotifyEvent*> NotifyStateBeginEvent;
    TArray<const FAnimNotifyEventReference*> NotifyStateBeginEventReference;

    for (int32 Index = 0; Index < NotifyQueue.AnimNotifies.Num(); Index++) {
        const FAnimNotifyEvent* AnimNotifyEvent = NotifyQueue.AnimNotifies[Index].GetNotify();
        if (AnimNotifyEvent == nullptr) continue;
        if (AnimNotifyEvent->NotifyStateClass == nullptr) {
            TriggerSingleAnimNotify(NotifyQueue.AnimNotifies[Index]);
            continue;
        }
        // A state that was active stays active; the others begin.
        const int32 ExistingItemIndex = ActiveAnimNotifyState.Find(*AnimNotifyEvent);
        if (ExistingItemIndex != INDEX_NONE) {
            ActiveAnimNotifyState.RemoveAtSwap(ExistingItemIndex);
            ActiveAnimNotifyEventReference.RemoveAtSwap(ExistingItemIndex);
        } else {
            NotifyStateBeginEvent.Add(AnimNotifyEvent);
            NotifyStateBeginEventReference.Add(&NotifyQueue.AnimNotifies[Index]);
        }
        NewActiveAnimNotifyState.Add(*AnimNotifyEvent);
        NewActiveAnimNotifyEventReference.Add(NotifyQueue.AnimNotifies[Index]);
    }
    // The references point at the copies of the states, which outlive the queue.
    for (int32 Index = 0; Index < NewActiveAnimNotifyState.Num(); Index++) NewActiveAnimNotifyEventReference[Index].SetNotify(&NewActiveAnimNotifyState[Index]);

    // The states that were active and are not in the queue end.
    for (int32 Index = 0; Index < ActiveAnimNotifyState.Num(); ++Index) {
        const FAnimNotifyEvent& AnimNotifyEvent = ActiveAnimNotifyState[Index];
        const FAnimNotifyEventReference& EventReference = ActiveAnimNotifyEventReference[Index];
        if (AnimNotifyEvent.NotifyStateClass && ShouldTriggerAnimNotifyState(AnimNotifyEvent.NotifyStateClass))
            AnimNotifyEvent.NotifyStateClass->NotifyEnd(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent.NotifyStateClass->GetOuter()), EventReference);
        // A NotifyEnd that changed the active states leaves them to the next frame (UE ensures here).
        if (!ActiveAnimNotifyState.IsValidIndex(Index)) return;
    }

    for (int32 Index = 0; Index < NotifyStateBeginEvent.Num(); Index++) {
        const FAnimNotifyEvent* AnimNotifyEvent = NotifyStateBeginEvent[Index];
        if (!ShouldTriggerAnimNotifyState(AnimNotifyEvent->NotifyStateClass)) continue;
        AnimNotifyEvent->NotifyStateClass->NotifyBegin(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent->NotifyStateClass->GetOuter()),
                                                       AnimNotifyEvent->GetDuration(), *NotifyStateBeginEventReference[Index]);
    }

    ActiveAnimNotifyState = NewActiveAnimNotifyState;
    ActiveAnimNotifyEventReference = NewActiveAnimNotifyEventReference;
    for (int32 Index = 0; Index < ActiveAnimNotifyEventReference.Num(); Index++) ActiveAnimNotifyEventReference[Index].SetNotify(&ActiveAnimNotifyState[Index]);

    for (int32 Index = 0; Index < ActiveAnimNotifyState.Num(); Index++) {
        const FAnimNotifyEvent& AnimNotifyEvent = ActiveAnimNotifyState[Index];
        if (!ShouldTriggerAnimNotifyState(AnimNotifyEvent.NotifyStateClass)) continue;
        AnimNotifyEvent.NotifyStateClass->NotifyTick(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent.NotifyStateClass->GetOuter()), DeltaSeconds,
                                                     ActiveAnimNotifyEventReference[Index]);
    }
}

void UAnimInstance::TriggerSingleAnimNotify(const FAnimNotifyEvent* AnimNotifyEvent) {
    FAnimNotifyEventReference EventReference(AnimNotifyEvent, this);
    TriggerSingleAnimNotify(EventReference);
}

void UAnimInstance::TriggerSingleAnimNotify(FAnimNotifyEventReference& EventReference) {
    const FAnimNotifyEvent* AnimNotifyEvent = EventReference.GetNotify();
    if (AnimNotifyEvent == nullptr || AnimNotifyEvent->NotifyStateClass != nullptr) return;
    if (HandleNotify(*AnimNotifyEvent)) return;
    if (AnimNotifyEvent->Notify != nullptr) {
        AnimNotifyEvent->Notify->Notify(GetSkelMeshComponent(), Cast<UAnimSequenceBase>(AnimNotifyEvent->Notify->GetOuter()), EventReference);
        return;
    }
    if (AnimNotifyEvent->NotifyName.isNone()) return;
    // A named notify calls AnimNotify_<Name> on this instance, and on the linked instances that take this one's.
    const FName FuncName = AnimNotifyEvent->GetNotifyEventName();
    auto NotifyAnimInstance = [this, AnimNotifyEvent, &FuncName](UAnimInstance* InAnimInstance) {
        if (InAnimInstance != this && !InAnimInstance->bReceiveNotifiesFromLinkedInstances) return;
        UFunction* Function = InAnimInstance->FindFunction(FuncName);
        if (Function == nullptr) return;
        if (Function->NumParms == 0) {
            InAnimInstance->ProcessEvent(Function, nullptr);
        } else if (Function->NumParms == 1 && CastField<FObjectProperty>(Function->PropertyLink) != nullptr) {
            struct FAnimNotifierHandler_Parms {
                UAnimNotify* Notify;
            };
            FAnimNotifierHandler_Parms Parms{AnimNotifyEvent->Notify};
            InAnimInstance->ProcessEvent(Function, &Parms);
        }
    };
    if (bPropagateNotifiesToLinkedInstances) GetSkelMeshComponent()->ForEachAnimInstance(NotifyAnimInstance);
    else NotifyAnimInstance(this);
}

void UAnimInstance::EndNotifyStates() {
    USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();
    for (int32 Index = 0; Index < ActiveAnimNotifyState.Num(); Index++) {
        const FAnimNotifyEvent& Event = ActiveAnimNotifyState[Index];
        if (UAnimNotifyState* NotifyState = Event.NotifyStateClass)
            NotifyState->NotifyEnd(SkelMeshComp, Cast<UAnimSequenceBase>(NotifyState->GetOuter()), ActiveAnimNotifyEventReference[Index]);
    }
    ActiveAnimNotifyState.Reset();
    ActiveAnimNotifyEventReference.Reset();
}

float UAnimInstance::GetRelevantAnimTimeRemaining(int32 MachineIndex, int32 StateIndex) {
    return AnimInstanceProxy->GetRelevantAnimTimeRemaining(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimTimeRemainingFraction(int32 MachineIndex, int32 StateIndex) {
    return AnimInstanceProxy->GetRelevantAnimTimeRemainingFraction(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimLength(int32 MachineIndex, int32 StateIndex) {
    return AnimInstanceProxy->GetRelevantAnimLength(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimTime(int32 MachineIndex, int32 StateIndex) {
    return AnimInstanceProxy->GetRelevantAnimTime(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimTimeFraction(int32 MachineIndex, int32 StateIndex) {
    return AnimInstanceProxy->GetRelevantAnimTimeFraction(MachineIndex, StateIndex);
}

float UAnimInstance::GetInstanceMachineWeight(int32 MachineIndex) { return AnimInstanceProxy->GetInstanceMachineWeight(MachineIndex); }

float UAnimInstance::GetInstanceStateWeight(int32 MachineIndex, int32 StateIndex) {
    return AnimInstanceProxy->GetInstanceStateWeight(MachineIndex, StateIndex);
}

float UAnimInstance::GetInstanceCurrentStateElapsedTime(int32 MachineIndex) {
    return AnimInstanceProxy->GetInstanceCurrentStateElapsedTime(MachineIndex);
}

void UAnimInstance::ParallelEvaluateAnimation(FPoseContext& Output) {
    AnimInstanceProxy->EvaluateAnimation(Output);
}

void UAnimInstance::PostEvaluateAnimation() {
    NativePostEvaluateAnimation();
    BlueprintPostEvaluateAnimation();
}

FAnimNode_LinkedInputPose* UAnimInstance::GetLinkedInputPoseNode() const { return AnimInstanceProxy->GetDefaultLinkedInstanceInputNode(); }

bool UAnimInstance::ParallelCanEvaluate(const USkeletalMesh* InSkeletalMesh) const {
    const FBoneContainer& RequiredBones = AnimInstanceProxy->GetRequiredBones();
    return RequiredBones.IsValid() && RequiredBones.GetAsset() == InSkeletalMesh;
}

void UAnimInstance::DispatchQueuedAnimEvents() {
    TriggerAnimNotifies(GetProxyOnGameThread().GetDeltaSeconds());
    TriggerQueuedMontageEvents();
    // An event can start or stop montages, so the array is read by index.
    for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++) {
        if (MontageInstances[InstanceIndex]->IsValid()) continue;
        ClearMontageInstanceReferences(*MontageInstances[InstanceIndex]);
        MontageInstances.RemoveAt(InstanceIndex);
        --InstanceIndex;
        if (MontageInstances.Num() == 0) OnAllMontageInstancesEnded.ProcessMulticastDelegate(nullptr);
    }
}

void UAnimInstance::UpdateMontage(float DeltaSeconds) {
    Montage_UpdateWeight(DeltaSeconds);
    Montage_Advance(DeltaSeconds);
}

void UAnimInstance::Montage_UpdateWeight(float DeltaSeconds) {
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances) MontageInstance->UpdateWeight(DeltaSeconds);
}

void UAnimInstance::Montage_Advance(float DeltaSeconds) {
    // The events of the advance wait for the dispatch after the evaluation.
    bQueueMontageEvents = true;
    for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++) {
        FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex].get();
        if (!MontageInstance->IsValid()) continue;
        MontageInstance->Advance(DeltaSeconds);
        if (!MontageInstances.IsValidIndex(InstanceIndex) || MontageInstances[InstanceIndex].get() != MontageInstance) break;
    }
}

void UAnimInstance::UpdateMontageEvaluationData() {
    TArray<FMontageEvaluationState>& EvaluationData = GetProxyOnGameThread().GetMontageEvaluationData();
    EvaluationData.Empty();
    // A montage with no weight is not evaluated, although it advances.
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances) {
        if (MontageInstance->Montage == nullptr || MontageInstance->GetWeight() <= ZERO_ANIMWEIGHT_THRESH) continue;
        EvaluationData.Add(FMontageEvaluationState{MontageInstance->Montage, MontageInstance->GetBlend(), MontageInstance->GetActiveBlendProfile(),
                                                   MontageInstance->GetPosition(), MontageInstance->DeltaTimeRecord,
                                                   MontageInstance->GetBlendStartAlpha(), MontageInstance->bPlaying, MontageInstance->IsActive()});
    }
}

float UAnimInstance::Montage_PlayInternal(UAnimMontage* MontageToPlay, const FMontageBlendSettings& BlendInSettings, float InPlayRate,
                                          EMontagePlayReturnType ReturnValueType, float InTimeToStartMontageAt, bool bStopAllMontages) {
    if (MontageToPlay == nullptr || MontageToPlay->GetPlayLength() <= 0.0f || !MontageToPlay->HasValidSlotSetup()) return 0.0f;
    if (CurrentSkeleton == nullptr || MontageToPlay->GetSkeleton() == nullptr) return 0.0f;
    if (MontageToPlay->TimeStretchCurve.IsValid()) ReportLoadProblem("anim.montage_time_stretch", MontageToPlay->GetPathName());
    if (BlendInSettings.BlendProfile != nullptr || MontageToPlay->BlendProfileOut != nullptr)
        ReportLoadProblem("anim.montage_blend_profile", MontageToPlay->GetPathName());
    // One montage at a time per slot group.
    if (bStopAllMontages) StopAllMontagesByGroupName(MontageToPlay->GetGroupName(), BlendInSettings);
    // One root motion montage at a time.
    if ((MontageToPlay->bEnableRootMotionTranslation || MontageToPlay->bEnableRootMotionRotation) && RootMotionMontageInstance != nullptr)
        RootMotionMontageInstance->Stop(BlendInSettings);

    FAnimMontageInstance* NewInstance = MontageInstances.Emplace_GetRef(std::make_unique<FAnimMontageInstance>(this)).get();
    const float MontageLength = MontageToPlay->GetPlayLength();
    NewInstance->Initialize(MontageToPlay);
    NewInstance->Play(InPlayRate, BlendInSettings);
    NewInstance->SetPosition(FMath::Clamp(InTimeToStartMontageAt, 0.0f, MontageLength));
    ActiveMontagesMap.Add(MontageToPlay, NewInstance);
    if (MontageToPlay->HasRootMotion()) RootMotionMontageInstance = NewInstance;
    OnMontageStarted.ProcessMulticastDelegate(&MontageToPlay);
    return ReturnValueType == EMontagePlayReturnType::MontageLength ? MontageLength : MontageLength / (InPlayRate * MontageToPlay->RateScale);
}

float UAnimInstance::Montage_Play(UAnimMontage* MontageToPlay, float InPlayRate, EMontagePlayReturnType ReturnValueType, float InTimeToStartMontageAt,
                                  bool bStopAllMontages) {
    FMontageBlendSettings BlendSettings;
    if (MontageToPlay != nullptr) {
        BlendSettings.Blend = FAlphaBlendArgs(MontageToPlay->BlendIn);
        BlendSettings.BlendMode = MontageToPlay->BlendModeIn;
        BlendSettings.BlendProfile = MontageToPlay->BlendProfileIn;
    }
    return Montage_PlayInternal(MontageToPlay, BlendSettings, InPlayRate, ReturnValueType, InTimeToStartMontageAt, bStopAllMontages);
}

float UAnimInstance::Montage_PlayWithBlendIn(UAnimMontage* MontageToPlay, const FAlphaBlendArgs& BlendIn, float InPlayRate,
                                             EMontagePlayReturnType ReturnValueType, float InTimeToStartMontageAt, bool bStopAllMontages) {
    FMontageBlendSettings BlendSettings;
    BlendSettings.Blend = BlendIn;
    if (MontageToPlay != nullptr) {
        BlendSettings.BlendMode = MontageToPlay->BlendModeIn;
        BlendSettings.BlendProfile = MontageToPlay->BlendProfileIn;
    }
    return Montage_PlayInternal(MontageToPlay, BlendSettings, InPlayRate, ReturnValueType, InTimeToStartMontageAt, bStopAllMontages);
}

float UAnimInstance::Montage_PlayWithBlendSettings(UAnimMontage* MontageToPlay, const FMontageBlendSettings& BlendInSettings, float InPlayRate,
                                                   EMontagePlayReturnType ReturnValueType, float InTimeToStartMontageAt, bool bStopAllMontages) {
    return Montage_PlayInternal(MontageToPlay, BlendInSettings, InPlayRate, ReturnValueType, InTimeToStartMontageAt, bStopAllMontages);
}

template <class Function>
void UAnimInstance::ForEachActiveInstance(const UAnimMontage* Montage, Function&& Visit) {
    if (Montage != nullptr) {
        if (FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage)) Visit(*MontageInstance);
        return;
    }
    for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
        if (MontageInstances[InstanceIndex]->IsActive()) Visit(*MontageInstances[InstanceIndex]);
}

const FAnimMontageInstance* UAnimInstance::FindQueriedInstance(const UAnimMontage* Montage) const {
    if (Montage != nullptr) return GetActiveInstanceForMontage(Montage);
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances)
        if (MontageInstance->IsActive()) return MontageInstance.get();
    return nullptr;
}

template <class BlendSelector>
void UAnimInstance::Montage_StopInternal(BlendSelector&& AlphaBlendSelectorFunction, const UAnimMontage* Montage) {
    ForEachActiveInstance(Montage, [&](FAnimMontageInstance& MontageInstance) { MontageInstance.Stop(AlphaBlendSelectorFunction(MontageInstance)); });
}

void UAnimInstance::ResetAnimGraph() { GetProxyOnGameThread().InitializeRootNode(false); }

void UAnimInstance::Montage_Stop(float InBlendOutTime, const UAnimMontage* Montage) {
    // The montage's blend out settings with another time.
    Montage_StopInternal(
        [InBlendOutTime](const FAnimMontageInstance& InMontageInstance) {
            FMontageBlendSettings BlendOutSettings;
            BlendOutSettings.Blend = FAlphaBlendArgs(InMontageInstance.Montage->BlendOut);
            BlendOutSettings.Blend.BlendTime = InBlendOutTime;
            BlendOutSettings.BlendMode = InMontageInstance.Montage->BlendModeOut;
            BlendOutSettings.BlendProfile = InMontageInstance.Montage->BlendProfileOut;
            return BlendOutSettings;
        },
        Montage);
}

void UAnimInstance::Montage_StopWithBlendOut(const FAlphaBlendArgs& BlendOut, const UAnimMontage* Montage) {
    Montage_StopInternal(
        [&BlendOut](const FAnimMontageInstance& InMontageInstance) {
            FMontageBlendSettings BlendOutSettings;
            BlendOutSettings.Blend = BlendOut;
            BlendOutSettings.BlendMode = InMontageInstance.Montage->BlendModeOut;
            BlendOutSettings.BlendProfile = InMontageInstance.Montage->BlendProfileOut;
            return BlendOutSettings;
        },
        Montage);
}

void UAnimInstance::Montage_StopWithBlendSettings(const FMontageBlendSettings& BlendOutSettings, const UAnimMontage* Montage) {
    Montage_StopInternal([&BlendOutSettings](const FAnimMontageInstance&) { return BlendOutSettings; }, Montage);
}

void UAnimInstance::Montage_StopGroupByName(float InBlendOutTime, FName GroupName) {
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances)
        if (MontageInstance->IsActive() && MontageInstance->Montage->GetGroupName() == GroupName)
            MontageInstance->Stop(FAlphaBlend(MontageInstance->Montage->BlendOut, InBlendOutTime));
}

void UAnimInstance::Montage_Pause(const UAnimMontage* Montage) {
    ForEachActiveInstance(Montage, [](FAnimMontageInstance& MontageInstance) { MontageInstance.Pause(); });
}

void UAnimInstance::Montage_Resume(const UAnimMontage* Montage) {
    ForEachActiveInstance(Montage, [](FAnimMontageInstance& MontageInstance) { MontageInstance.SetPlaying(true); });
}

void UAnimInstance::Montage_JumpToSection(FName SectionName, const UAnimMontage* Montage) {
    // A montage that plays backwards jumps to the section's end.
    ForEachActiveInstance(Montage, [SectionName](FAnimMontageInstance& MontageInstance) {
        MontageInstance.JumpToSectionName(SectionName, MontageInstance.GetPlayRate() < 0.0f);
    });
}

void UAnimInstance::Montage_JumpToSectionsEnd(FName SectionName, const UAnimMontage* Montage) {
    ForEachActiveInstance(Montage, [SectionName](FAnimMontageInstance& MontageInstance) {
        MontageInstance.JumpToSectionName(SectionName, MontageInstance.GetPlayRate() >= 0.0f);
    });
}

void UAnimInstance::Montage_SetNextSection(FName SectionNameToChange, FName NextSection, const UAnimMontage* Montage) {
    ForEachActiveInstance(Montage, [&](FAnimMontageInstance& MontageInstance) { MontageInstance.SetNextSectionName(SectionNameToChange, NextSection); });
}

void UAnimInstance::Montage_SetPlayRate(const UAnimMontage* Montage, float NewPlayRate) {
    ForEachActiveInstance(Montage, [NewPlayRate](FAnimMontageInstance& MontageInstance) { MontageInstance.SetPlayRate(NewPlayRate); });
}

void UAnimInstance::Montage_SetPosition(const UAnimMontage* Montage, float NewPosition) {
    ForEachActiveInstance(Montage, [NewPosition](FAnimMontageInstance& MontageInstance) { MontageInstance.SetPosition(NewPosition); });
}

void UAnimInstance::Montage_SetEndDelegate(FOnMontageEnded& InOnMontageEnded, UAnimMontage* Montage) {
    ForEachActiveInstance(Montage, [&InOnMontageEnded](FAnimMontageInstance& MontageInstance) { MontageInstance.OnMontageEnded = InOnMontageEnded; });
}

void UAnimInstance::Montage_SetBlendingOutDelegate(FOnMontageBlendingOutStarted& InOnMontageBlendingOut, UAnimMontage* Montage) {
    ForEachActiveInstance(Montage, [&InOnMontageBlendingOut](FAnimMontageInstance& MontageInstance) {
        MontageInstance.OnMontageBlendingOutStarted = InOnMontageBlendingOut;
    });
}

bool UAnimInstance::Montage_IsActive(const UAnimMontage* Montage) const { return FindQueriedInstance(Montage) != nullptr; }

bool UAnimInstance::Montage_IsPlaying(const UAnimMontage* Montage) const {
    if (Montage != nullptr) {
        const FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
        return MontageInstance != nullptr && MontageInstance->IsPlaying();
    }
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances)
        if (MontageInstance->IsActive() && MontageInstance->IsPlaying()) return true;
    return false;
}

FName UAnimInstance::Montage_GetCurrentSection(const UAnimMontage* Montage) const {
    const FAnimMontageInstance* MontageInstance = FindQueriedInstance(Montage);
    return MontageInstance ? MontageInstance->GetCurrentSection() : FName();
}

float UAnimInstance::Montage_GetPosition(const UAnimMontage* Montage) const {
    const FAnimMontageInstance* MontageInstance = FindQueriedInstance(Montage);
    return MontageInstance ? MontageInstance->GetPosition() : 0.0f;
}

bool UAnimInstance::Montage_GetIsStopped(const UAnimMontage* Montage) const {
    return Montage == nullptr || GetActiveInstanceForMontage(Montage) == nullptr;
}

float UAnimInstance::Montage_GetBlendTime(const UAnimMontage* Montage) const {
    const FAnimMontageInstance* MontageInstance = FindQueriedInstance(Montage);
    return MontageInstance ? MontageInstance->GetBlendTime() : 0.0f;
}

float UAnimInstance::Montage_GetPlayRate(const UAnimMontage* Montage) const {
    const FAnimMontageInstance* MontageInstance = FindQueriedInstance(Montage);
    return MontageInstance ? MontageInstance->GetPlayRate() : 0.0f;
}

float UAnimInstance::Montage_GetEffectivePlayRate(const UAnimMontage* Montage) const {
    const FAnimMontageInstance* MontageInstance = FindQueriedInstance(Montage);
    return MontageInstance ? MontageInstance->GetPlayRate() * MontageInstance->Montage->RateScale : 0.0f;
}

UAnimMontage* UAnimInstance::GetCurrentActiveMontage() const {
    const FAnimMontageInstance* MontageInstance = GetActiveMontageInstance();
    return MontageInstance ? MontageInstance->Montage : nullptr;
}

FAnimMontageInstance* UAnimInstance::GetActiveMontageInstance() const {
    // The newest instances are at the end.
    for (int32 InstanceIndex = MontageInstances.Num() - 1; InstanceIndex >= 0; InstanceIndex--)
        if (MontageInstances[InstanceIndex]->IsActive()) return MontageInstances[InstanceIndex].get();
    return nullptr;
}

FAnimMontageInstance* UAnimInstance::GetActiveInstanceForMontage(const UAnimMontage* Montage) const {
    FAnimMontageInstance* const* Found = ActiveMontagesMap.Find(Montage);
    return Found ? *Found : nullptr;
}

void UAnimInstance::StopAllMontages(float BlendOut) {
    for (int32 Index = MontageInstances.Num() - 1; Index >= 0; Index--) MontageInstances[Index]->Stop(FAlphaBlend(BlendOut), true);
}

void UAnimInstance::StopAllMontagesByGroupName(FName InGroupName, const FMontageBlendSettings& BlendOutSettings) {
    for (int32 InstanceIndex = MontageInstances.Num() - 1; InstanceIndex >= 0; InstanceIndex--) {
        FAnimMontageInstance& MontageInstance = *MontageInstances[InstanceIndex];
        if (MontageInstance.Montage != nullptr && MontageInstance.Montage->GetGroupName() == InGroupName) MontageInstance.Stop(BlendOutSettings, true);
    }
}

UAnimMontage* UAnimInstance::PlaySlotAnimationAsDynamicMontage(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime, float BlendOutTime,
                                                               float InPlayRate, int32 LoopCount, float BlendOutTriggerTime,
                                                               float InTimeToStartMontageAt) {
    return PlaySlotAnimationAsDynamicMontage_WithBlendSettings(Asset, SlotNodeName, FMontageBlendSettings(BlendInTime), FMontageBlendSettings(BlendOutTime),
                                                               InPlayRate, LoopCount, BlendOutTriggerTime, InTimeToStartMontageAt);
}

UAnimMontage* UAnimInstance::PlaySlotAnimationAsDynamicMontage_WithBlendArgs(UAnimSequenceBase* Asset, FName SlotNodeName,
                                                                             const FAlphaBlendArgs& BlendIn, const FAlphaBlendArgs& BlendOut,
                                                                             float InPlayRate, int32 LoopCount, float BlendOutTriggerTime,
                                                                             float InTimeToStartMontageAt) {
    return PlaySlotAnimationAsDynamicMontage_WithBlendSettings(Asset, SlotNodeName, FMontageBlendSettings(BlendIn), FMontageBlendSettings(BlendOut),
                                                               InPlayRate, LoopCount, BlendOutTriggerTime, InTimeToStartMontageAt);
}

UAnimMontage* UAnimInstance::PlaySlotAnimationAsDynamicMontage_WithBlendSettings(UAnimSequenceBase* Asset, FName SlotNodeName,
                                                                                 const FMontageBlendSettings& BlendInSettings,
                                                                                 const FMontageBlendSettings& BlendOutSettings, float InPlayRate,
                                                                                 int32 LoopCount, float BlendOutTriggerTime,
                                                                                 float InTimeToStartMontageAt) {
    if (Asset == nullptr || Asset->GetSkeleton() == nullptr) return nullptr;
    UAnimMontage* NewMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage_WithBlendSettings(
        Asset, SlotNodeName, BlendInSettings, BlendOutSettings, InPlayRate, LoopCount, BlendOutTriggerTime);
    if (NewMontage == nullptr) return nullptr;
    const float PlayTime = Montage_Play(NewMontage, InPlayRate, EMontagePlayReturnType::MontageLength, InTimeToStartMontageAt);
    return PlayTime > 0.0f ? NewMontage : nullptr;
}

void UAnimInstance::StopSlotAnimation(float InBlendOutTime, FName SlotNodeName) {
    if (SlotNodeName.isNone()) {
        Montage_Stop(InBlendOutTime);
        return;
    }
    // Only the transient montages of PlaySlotAnimationAsDynamicMontage.
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances) {
        if (!MontageInstance->IsActive() || !MontageInstance->IsPlaying()) continue;
        const UAnimMontage* CurMontage = MontageInstance->Montage;
        if (CurMontage->GetOuter() != GetTransientPackage()) continue;
        for (const FSlotAnimationTrack& AnimTrack : CurMontage->SlotAnimTracks) {
            if (AnimTrack.SlotName != SlotNodeName) continue;
            MontageInstance->Stop(FAlphaBlend(InBlendOutTime));
            break;
        }
    }
}

bool UAnimInstance::IsPlayingSlotAnimation(const UAnimSequenceBase* Asset, FName SlotNodeName) const {
    for (const std::unique_ptr<FAnimMontageInstance>& MontageInstance : MontageInstances) {
        if (!MontageInstance->IsActive() || !MontageInstance->IsPlaying()) continue;
        const UAnimMontage* CurMontage = MontageInstance->Montage;
        if (CurMontage->GetOuter() != GetTransientPackage()) continue;
        const FAnimTrack* AnimTrack = CurMontage->GetAnimationData(SlotNodeName);
        if (AnimTrack != nullptr && AnimTrack->AnimSegments.Num() == 1) return AnimTrack->AnimSegments[0].GetAnimReference() == Asset;
    }
    return false;
}

bool UAnimInstance::IsSlotActive(FName SlotNodeName) const {
    if (SlotNodeName.isNone()) return false;
    for (const FMontageEvaluationState& EvaluationState : AnimInstanceProxy->GetMontageEvaluationData())
        if (EvaluationState.bIsActive && EvaluationState.Montage->IsValidSlot(SlotNodeName)) return true;
    return false;
}

float UAnimInstance::Blueprint_GetSlotMontageLocalWeight(FName SlotNodeName) const { return AnimInstanceProxy->GetSlotMontageLocalWeight(SlotNodeName); }

float UAnimInstance::GetSlotNodeGlobalWeight(const FName& SlotNodeName) const { return GetProxyOnGameThread().GetSlotNodeGlobalWeight(SlotNodeName); }

float UAnimInstance::GetSlotMontageGlobalWeight(const FName& SlotNodeName) const { return AnimInstanceProxy->GetSlotMontageGlobalWeight(SlotNodeName); }

void UAnimInstance::ClearMontageInstanceReferences(FAnimMontageInstance& InMontageInstance) {
    if (const UAnimMontage* MontageStopped = InMontageInstance.Montage) {
        FAnimMontageInstance** AnimInstancePtr = ActiveMontagesMap.Find(MontageStopped);
        if (AnimInstancePtr != nullptr && *AnimInstancePtr == &InMontageInstance) ActiveMontagesMap.Remove(MontageStopped);
    }
    if (RootMotionMontageInstance == &InMontageInstance) RootMotionMontageInstance = nullptr;
}

void UAnimInstance::QueueMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent) {
    if (bQueueMontageEvents) {
        QueuedMontageBlendingOutEvents.Add(MontageBlendingOutEvent);
        return;
    }
    TriggerMontageBlendingOutEvent(MontageBlendingOutEvent);
}

void UAnimInstance::QueueMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent) {
    if (bQueueMontageEvents) {
        QueuedMontageEndedEvents.Add(MontageEndedEvent);
        return;
    }
    TriggerMontageEndedEvent(MontageEndedEvent);
}

void UAnimInstance::TriggerMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent) {
    if (MontageBlendingOutEvent.Delegate) MontageBlendingOutEvent.Delegate(MontageBlendingOutEvent.Montage, MontageBlendingOutEvent.bInterrupted);
    struct {
        UAnimMontage* Montage;
        bool bInterrupted;
    } Parms{MontageBlendingOutEvent.Montage, MontageBlendingOutEvent.bInterrupted};
    OnMontageBlendingOut.ProcessMulticastDelegate(&Parms);
}

void UAnimInstance::TriggerMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent) {
    // UE also ends the montage's active notify states here; they come with the notify queue.
    if (MontageEndedEvent.Delegate) MontageEndedEvent.Delegate(MontageEndedEvent.Montage, MontageEndedEvent.bInterrupted);
    struct {
        UAnimMontage* Montage;
        bool bInterrupted;
    } Parms{MontageEndedEvent.Montage, MontageEndedEvent.bInterrupted};
    OnMontageEnded.ProcessMulticastDelegate(&Parms);
}

void UAnimInstance::TriggerQueuedMontageEvents() {
    bQueueMontageEvents = false;
    // The blending out events come before the ended events. A handler can queue more, so the lists are moved out.
    TArray<FQueuedMontageBlendingOutEvent> BlendingOutEvents = std::move(QueuedMontageBlendingOutEvents);
    QueuedMontageBlendingOutEvents.Empty();
    for (const FQueuedMontageBlendingOutEvent& Event : BlendingOutEvents) TriggerMontageBlendingOutEvent(Event);
    TArray<FQueuedMontageEndedEvent> EndedEvents = std::move(QueuedMontageEndedEvents);
    QueuedMontageEndedEvents.Empty();
    for (const FQueuedMontageEndedEvent& Event : EndedEvents) TriggerMontageEndedEvent(Event);
}

}
