#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

#include "ue5parse/runtime/Core/Math/UnrealMathUtility.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerRelevancyBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_LinkedInputPose.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_Root.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_SaveCachedPose.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_StateMachine.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

namespace ue5parse::runtime {

void FAnimInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance) {
    SkeletalMeshComponent = InAnimInstance->GetSkelMeshComponent();
    UAnimInstance* MainAnimInstance = SkeletalMeshComponent->GetAnimInstance();
    MainInstanceProxy = MainAnimInstance != nullptr && MainAnimInstance != InAnimInstance ? &MainAnimInstance->GetProxyOnAnyThread() : nullptr;
    const USkeletalMesh* Mesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
    Skeleton = Mesh != nullptr ? Mesh->GetSkeleton() : nullptr;
}

void FAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance) {
    AnimInstanceObject = InAnimInstance;
    AnimClassInterface = Cast<UAnimBlueprintGeneratedClass>(InAnimInstance->GetClass());
    InitializeObjects(InAnimInstance);
    RootNode = nullptr;
    DefaultLinkedInstanceInputNode = nullptr;
    bShouldExtractRootMotion = InAnimInstance->ShouldExtractRootMotion();
    SavedPoseQueue.Empty();
    GameThreadPreUpdateNodes.Empty();
    UpdateCounter.Reset();
    ReinitializeSlotNodes();
    if (AnimClassInterface == nullptr) return;

    // The nodes' table entries belong to the class that compiled the graph; a child class without its own graph uses
    // its parent's.
    const UAnimBlueprintGeneratedClass* DataClass = AnimClassInterface->GetNodeDataClass();
    const TArray<FStructProperty*>& AnimNodeProperties = AnimClassInterface->GetAnimNodeProperties();
    for (int32 NodeIndex = 0; NodeIndex < AnimNodeProperties.Num(); ++NodeIndex) {
        if (AnimClassInterface->GetLinkTarget(NodeIndex) != NodeIndex) continue;
        FAnimNode_Base* Node = AnimNodeProperties[NodeIndex]->ContainerPtrToValuePtr<FAnimNode_Base>(InAnimInstance);
        Node->SetNodeData(DataClass->AnimNodeData[NodeIndex]);
        if (Node->HasPreUpdate()) GameThreadPreUpdateNodes.Add(Node);
        if (Node->NeedsOnInitializeAnimInstance()) Node->OnInitializeAnimInstance(this, InAnimInstance);
    }

    // The root of the main graph: the root node named AnimGraph, else the first root node.
    for (int32 NodeIndex = 0; NodeIndex < AnimNodeProperties.Num(); ++NodeIndex) {
        if (AnimNodeProperties[NodeIndex]->Struct != FAnimNode_Root::StaticStruct()) continue;
        auto* Root = AnimNodeProperties[NodeIndex]->ContainerPtrToValuePtr<FAnimNode_Root>(InAnimInstance);
        if (RootNode == nullptr || Root->GetName() == FName("AnimGraph")) RootNode = Root;
    }

    for (const FAnimBlueprintFunction& Function : AnimClassInterface->GetAnimBlueprintFunctions()) {
        if (Function.Name != FName("AnimGraph") || Function.InputPoseNodeProperties.Num() == 0 || Function.InputPoseNodeProperties[0] == nullptr) continue;
        DefaultLinkedInstanceInputNode = Function.InputPoseNodeProperties[0]->ContainerPtrToValuePtr<FAnimNode_LinkedInputPose>(InAnimInstance);
        break;
    }

    if (const TArray<int32>* Ordered = AnimClassInterface->GetOrderedSavedPoseNodeIndices(FName("AnimGraph")))
        for (int32 Index : *Ordered) SavedPoseQueue.Add(GetMutableNodeFromIndex<FAnimNode_SaveCachedPose>(Index));

    // The state machines belong to the graph, so the root class has them: a child class with its own node data has none.
    const UAnimBlueprintGeneratedClass* RootClass = AnimClassInterface->GetRootAnimClass();
    for (int32 NodeIndex = 0; NodeIndex < AnimNodeProperties.Num(); ++NodeIndex) {
        if (AnimClassInterface->GetLinkTarget(NodeIndex) != NodeIndex || !AnimNodeProperties[NodeIndex]->Struct->IsChildOf(FAnimNode_StateMachine::StaticStruct()))
            continue;
        AnimNodeProperties[NodeIndex]->ContainerPtrToValuePtr<FAnimNode_StateMachine>(InAnimInstance)->CacheMachineDescription(RootClass);
    }
    const TArray<FBakedAnimationStateMachine>& Machines = RootClass->BakedStateMachines;
    StateMachineWeightOffsets.Empty();
    int32 NumStates = 0;
    for (const FBakedAnimationStateMachine& Machine : Machines) {
        StateMachineWeightOffsets.Add(NumStates);
        NumStates += Machine.States.Num();
    }
    for (int32 Buffer = 0; Buffer < 2; ++Buffer) {
        MachineWeightArrays[Buffer].Empty();
        MachineWeightArrays[Buffer].SetNum(Machines.Num());
        StateWeightArrays[Buffer].Empty();
        StateWeightArrays[Buffer].SetNum(NumStates);
    }
}

void FAnimInstanceProxy::InitializeRootNode(bool bInDeferRootNodeInitialization) {
    bBoneCachesInvalidated = true;
    if (bInDeferRootNodeInitialization) {
        bDeferRootNodeInitialization = true;
        return;
    }
    InitializeRootNode_WithRoot(RootNode);
}

void FAnimInstanceProxy::InitializeRootNode_WithRoot(FAnimNode_Base* InRootNode) {
    if (InRootNode == nullptr) return;
    if (InRootNode == RootNode) InitializationCounter.Increment();
    FAnimationInitializeContext InitContext(this);
    InRootNode->Initialize_AnyThread(InitContext);
}

void FAnimInstanceProxy::CacheBones_WithRoot(FAnimNode_Base* InRootNode) {
    if (!bBoneCachesInvalidated || InRootNode == nullptr) return;
    CacheBonesRecursionCounter++;
    if (InRootNode == RootNode) CachedBonesCounter.Increment();
    FAnimationCacheBonesContext Context(this);
    InRootNode->CacheBones_AnyThread(Context);
    CacheBonesRecursionCounter--;
    if (CacheBonesRecursionCounter == 0) bBoneCachesInvalidated = false;
}

void FAnimInstanceProxy::ThreadSafeUpdate() {
    if (bThreadSafeUpdateDone) return;
    bThreadSafeUpdateDone = true;
    ProcessCopies(EAnimPropertyAccessCallSite::WorkerThread_Batched_PreEventGraph);
    AnimInstanceObject->NativeThreadSafeUpdateAnimation(CurrentDeltaSeconds);
    AnimInstanceObject->BlueprintThreadSafeUpdateAnimation(CurrentDeltaSeconds);
    ProcessCopies(EAnimPropertyAccessCallSite::WorkerThread_Batched_PostEventGraph);
}

void FAnimInstanceProxy::UpdateAnimation_WithRoot(const FAnimationUpdateContext& InContext, FAnimNode_Base* InRootNode, FName InLayerName) {
    if (InRootNode == RootNode) {
        if (bDeferRootNodeInitialization) {
            bDeferRootNodeInitialization = false;
            InitializeRootNode_WithRoot(RootNode);
        }
        CacheBones();
    } else {
        CacheBones_WithRoot(InRootNode);
    }
    ThreadSafeUpdate();
    if (InRootNode == RootNode) {
        UpdateAnimationNode(InContext);
        return;
    }
    if (InRootNode == nullptr) return;
    InRootNode->Update_AnyThread(InContext);
}

void FAnimInstanceProxy::EvaluateAnimation_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) {
    if (InRootNode == RootNode) CacheBones();
    else CacheBones_WithRoot(InRootNode);
    if (InRootNode == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    if (InRootNode == RootNode) EvaluationCounter.Increment();
    InRootNode->Evaluate_AnyThread(Output);
}

void FAnimInstanceProxy::RecalcRequiredBones(USkeletalMeshComponent* Component, UObject* Asset) {
    RequiredBones.InitializeTo(Component->RequiredBones, *Asset);
    bBoneCachesInvalidated = true;
}

void FAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) {
    CurrentDeltaSeconds = DeltaSeconds;
    bShouldExtractRootMotion = InAnimInstance->ShouldExtractRootMotion();
    ComponentTransform = SkeletalMeshComponent->GetComponentTransform();
    ComponentRelativeTransform = SkeletalMeshComponent->GetRelativeTransform();
    const AActor* Owner = SkeletalMeshComponent->GetOwner();
    ActorTransform = Owner != nullptr ? Owner->GetActorTransform() : ComponentTransform;
    ClearSlotNodeWeights();
    Sync.Reset();
    for (float& Weight : MachineWeightArrays[BufferWriteIndex]) Weight = 0.0f;
    for (float& Weight : StateWeightArrays[BufferWriteIndex]) Weight = 0.0f;
    bThreadSafeUpdateDone = false;
    NotifyQueue.Reset(SkeletalMeshComponent);
    for (FAnimNode_Base* Node : GameThreadPreUpdateNodes) Node->PreUpdate(InAnimInstance);
}

void FAnimInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) {
    BufferWriteIndex = 1 - BufferWriteIndex;
    InAnimInstance->NotifyQueue.Append(NotifyQueue);
    InAnimInstance->NotifyQueue.ApplyMontageNotifies(*this);
}

void FAnimInstanceProxy::AddAnimNotifyFromGeneratedClass(int32 NotifyIndex) {
    if (NotifyIndex == INDEX_NONE || AnimClassInterface == nullptr) return;
    const TArray<FAnimNotifyEvent>& Notifies = AnimClassInterface->GetRootAnimClass()->AnimNotifies;
    if (!Notifies.IsValidIndex(NotifyIndex)) return;
    NotifyQueue.AddAnimNotify(&Notifies[NotifyIndex], AnimClassInterface);
}

void FAnimInstanceProxy::ReinitializeSlotNodes() {
    SlotNameToTrackerIndex.Empty();
    SlotWeightTracker[0].Empty();
    SlotWeightTracker[1].Empty();
    SlotNodeInitializationCounter.Increment();
}

void FAnimInstanceProxy::RegisterSlotNodeWithAnimInstance(const FName& SlotNodeName) {
    if (SlotNameToTrackerIndex.Find(SlotNodeName) != nullptr) return;
    SlotNameToTrackerIndex.Add(SlotNodeName, SlotWeightTracker[0].Num());
    SlotWeightTracker[0].AddDefaulted();
    SlotWeightTracker[1].AddDefaulted();
}

void FAnimInstanceProxy::UpdateSlotNodeWeight(const FName& SlotNodeName, float InMontageLocalWeight, float InNodeGlobalWeight) {
    const int32* TrackerIndexPtr = SlotNameToTrackerIndex.Find(SlotNodeName);
    if (TrackerIndexPtr == nullptr) return;
    FMontageActiveSlotTracker& Tracker = SlotWeightTracker[BufferWriteIndex][*TrackerIndexPtr];
    Tracker.MontageLocalWeight = InMontageLocalWeight;
    Tracker.NodeGlobalWeight = InNodeGlobalWeight;
    Tracker.bIsRelevantThisTick = Tracker.bIsRelevantThisTick || FAnimWeight::IsRelevant(InMontageLocalWeight);
}

void FAnimInstanceProxy::ClearSlotNodeWeights() {
    const TArray<FMontageActiveSlotTracker>& SlotWeightTracker_Read = SlotWeightTracker[1 - BufferWriteIndex];
    TArray<FMontageActiveSlotTracker>& SlotWeightTracker_Write = SlotWeightTracker[BufferWriteIndex];
    for (int32 TrackerIndex = 0; TrackerIndex < SlotWeightTracker_Write.Num(); TrackerIndex++) {
        SlotWeightTracker_Write[TrackerIndex] = FMontageActiveSlotTracker();
        SlotWeightTracker_Write[TrackerIndex].bWasRelevantOnPreviousTick = SlotWeightTracker_Read[TrackerIndex].bIsRelevantThisTick;
    }
}

bool FAnimInstanceProxy::IsSlotNodeRelevantForNotifies(const FName& SlotNodeName) const {
    const int32* TrackerIndexPtr = SlotNameToTrackerIndex.Find(SlotNodeName);
    if (TrackerIndexPtr == nullptr) return false;
    const FMontageActiveSlotTracker& Tracker = SlotWeightTracker[1 - BufferWriteIndex][*TrackerIndexPtr];
    return Tracker.bIsRelevantThisTick || Tracker.bWasRelevantOnPreviousTick;
}

float FAnimInstanceProxy::GetSlotNodeGlobalWeight(const FName& SlotNodeName) const {
    const int32* TrackerIndexPtr = SlotNameToTrackerIndex.Find(SlotNodeName);
    return TrackerIndexPtr ? SlotWeightTracker[1 - BufferWriteIndex][*TrackerIndexPtr].NodeGlobalWeight : 0.0f;
}

float FAnimInstanceProxy::GetSlotMontageGlobalWeight(const FName& SlotNodeName) const {
    const int32* TrackerIndexPtr = SlotNameToTrackerIndex.Find(SlotNodeName);
    if (TrackerIndexPtr == nullptr) return 0.0f;
    const FMontageActiveSlotTracker& Tracker = SlotWeightTracker[1 - BufferWriteIndex][*TrackerIndexPtr];
    return Tracker.MontageLocalWeight * Tracker.NodeGlobalWeight;
}

float FAnimInstanceProxy::GetSlotMontageLocalWeight(const FName& SlotNodeName) const {
    const int32* TrackerIndexPtr = SlotNameToTrackerIndex.Find(SlotNodeName);
    return TrackerIndexPtr ? SlotWeightTracker[1 - BufferWriteIndex][*TrackerIndexPtr].MontageLocalWeight : 0.0f;
}

void FAnimInstanceProxy::GetSlotWeight(const FName& SlotNodeName, float& out_SlotNodeWeight, float& out_SourceWeight, float& out_TotalNodeWeight) const {
    float NewSlotNodeWeight = 0.0f;
    // An additive montage does not take weight from the source pose.
    float NonAdditiveTotalWeight = 0.0f;
    for (const FMontageEvaluationState& EvalState : MontageEvaluationData) {
        if (!EvalState.Montage->IsValidSlot(SlotNodeName)) continue;
        const float MontageWeight = EvalState.BlendInfo.GetBlendedValue();
        NewSlotNodeWeight += MontageWeight;
        if (!EvalState.Montage->IsValidAdditiveSlot(SlotNodeName)) NonAdditiveTotalWeight += MontageWeight;
    }
    out_TotalNodeWeight = NewSlotNodeWeight;
    // Overlapping blends can sum above 1: the evaluation normalizes the montages by the total.
    if (NewSlotNodeWeight > 1.0f) {
        NonAdditiveTotalWeight /= NewSlotNodeWeight;
        NewSlotNodeWeight = 1.0f;
    }
    out_SlotNodeWeight = NewSlotNodeWeight;
    out_SourceWeight = 1.0f - NonAdditiveTotalWeight;
}

void FAnimInstanceProxy::SlotEvaluatePose(const FName& SlotNodeName, const FAnimationPoseData& SourceAnimationPoseData, float InSourceWeight,
                                          FAnimationPoseData& OutBlendedAnimationPoseData, float InBlendWeight, float InTotalNodeWeight) {
    const FCompactPose& SourcePose = SourceAnimationPoseData.GetPose();
    const FBlendedCurve& SourceCurve = SourceAnimationPoseData.GetCurve();
    FCompactPose& BlendedPose = OutBlendedAnimationPoseData.GetPose();
    FBlendedCurve& BlendedCurve = OutBlendedAnimationPoseData.GetCurve();
    if (InBlendWeight <= ZERO_ANIMWEIGHT_THRESH) {
        BlendedPose = SourcePose;
        BlendedCurve = SourceCurve;
        return;
    }

    // The montages of the slot, split into non-additive and additive poses.
    struct FSlotEvaluationPose {
        EAdditiveAnimationType AdditiveType = AAT_None;
        float Weight = 0;
        FCompactPose Pose;
        FBlendedCurve Curve;
    };
    TArray<FSlotEvaluationPose> NonAdditivePoses;
    TArray<FSlotEvaluationPose> AdditivePoses;
    for (const FMontageEvaluationState& EvalState : MontageEvaluationData) {
        const UAnimMontage* Montage = EvalState.Montage;
        if (!Montage->IsValidSlot(SlotNodeName)) continue;
        const FAnimTrack* AnimTrack = Montage->GetAnimationData(SlotNodeName);
        const EAdditiveAnimationType AdditiveAnimType =
            AnimTrack->IsAdditive() ? (AnimTrack->IsRotationOffsetAdditive() ? AAT_RotationOffsetMeshSpace : AAT_LocalSpaceBase) : AAT_None;
        FSlotEvaluationPose& NewPose = (AdditiveAnimType == AAT_None ? NonAdditivePoses : AdditivePoses).Emplace_GetRef();
        NewPose.AdditiveType = AdditiveAnimType;
        NewPose.Weight = EvalState.BlendInfo.GetBlendedValue();
        NewPose.Pose.SetBoneContainer(&RequiredBones);
        FAnimationPoseData NewAnimationPoseData(NewPose.Pose, NewPose.Curve);
        const FAnimExtractContext ExtractionContext(static_cast<double>(EvalState.MontagePosition),
                                                    Montage->HasRootMotion() && AnimInstanceObject->RootMotionMode != ERootMotionMode::NoRootMotionExtraction,
                                                    EvalState.DeltaTimeRecord);
        AnimTrack->GetAnimationPose(NewAnimationPoseData, ExtractionContext);
        FBlendedCurve MontageCurve;
        Montage->EvaluateCurveData(MontageCurve, EvalState.MontagePosition);
        NewPose.Curve.Combine(MontageCurve);
    }

    // Overlapping blends: the montages share a weight of 1.
    if (InTotalNodeWeight > 1.0f + ZERO_ANIMWEIGHT_THRESH) {
        for (FSlotEvaluationPose& Pose : AdditivePoses) Pose.Weight /= InTotalNodeWeight;
        for (FSlotEvaluationPose& Pose : NonAdditivePoses) Pose.Weight /= InTotalNodeWeight;
    }

    const float SourceWeight = FMath::Clamp(InSourceWeight, 0.0f, 1.0f);
    if (NonAdditivePoses.Num() == 0) {
        BlendedPose = SourcePose;
        BlendedCurve = SourceCurve;
    } else {
        TArray<const FCompactPose*> BlendingPoses;
        TArray<const FBlendedCurve*> BlendingCurves;
        TArray<float> BlendingWeights;
        for (const FSlotEvaluationPose& Pose : NonAdditivePoses) {
            BlendingPoses.Add(&Pose.Pose);
            BlendingCurves.Add(&Pose.Curve);
            BlendingWeights.Add(Pose.Weight);
        }
        if (SourceWeight > ZERO_ANIMWEIGHT_THRESH) {
            BlendingPoses.Add(&SourcePose);
            BlendingCurves.Add(&SourceCurve);
            BlendingWeights.Add(SourceWeight);
        }
        FAnimationRuntime::BlendPosesTogetherIndirect(BlendingPoses, BlendingCurves, BlendingWeights, OutBlendedAnimationPoseData);
    }
    for (const FSlotEvaluationPose& AdditivePose : AdditivePoses) {
        const FAnimationPoseData AdditiveAnimationPoseData(const_cast<FCompactPose&>(AdditivePose.Pose), const_cast<FBlendedCurve&>(AdditivePose.Curve));
        FAnimationRuntime::AccumulateAdditivePose(OutBlendedAnimationPoseData, AdditiveAnimationPoseData, AdditivePose.Weight, AdditivePose.AdditiveType);
    }
}

void FAnimInstanceProxy::RecordMachineWeight(int32 InMachineClassIndex, float InMachineWeight) {
    MachineWeightArrays[BufferWriteIndex][InMachineClassIndex] = InMachineWeight;
}

void FAnimInstanceProxy::RecordStateWeight(int32 InMachineClassIndex, int32 InStateIndex, float InStateWeight) {
    StateWeightArrays[BufferWriteIndex][StateMachineWeightOffsets[InMachineClassIndex] + InStateIndex] = InStateWeight;
}

const FAnimNode_StateMachine* FAnimInstanceProxy::GetStateMachineInstance(int32 MachineIndex) const {
    return GetMutableNodeFromIndex<FAnimNode_StateMachine>(MachineIndex);
}

const FAnimNode_AssetPlayerRelevancyBase* FAnimInstanceProxy::GetRelevantAssetPlayerInterfaceFromState(int32 MachineIndex, int32 StateIndex) const {
    const FAnimNode_StateMachine* StateMachine = GetStateMachineInstance(MachineIndex);
    return StateMachine ? StateMachine->GetRelevantAssetPlayerInterfaceFromState(this, StateMachine->GetStateInfo(StateIndex)) : nullptr;
}

float FAnimInstanceProxy::GetRelevantAnimTimeRemaining(int32 MachineIndex, int32 StateIndex) const {
    const FAnimNode_StateMachine* StateMachine = GetStateMachineInstance(MachineIndex);
    return StateMachine ? StateMachine->GetRelevantAnimTimeRemaining(this, StateIndex) : UE_MAX_FLT;
}

float FAnimInstanceProxy::GetRelevantAnimTimeRemainingFraction(int32 MachineIndex, int32 StateIndex) const {
    const FAnimNode_StateMachine* StateMachine = GetStateMachineInstance(MachineIndex);
    return StateMachine ? StateMachine->GetRelevantAnimTimeRemainingFraction(this, StateIndex) : 1.0f;
}

float FAnimInstanceProxy::GetRelevantAnimLength(int32 MachineIndex, int32 StateIndex) const {
    const FAnimNode_AssetPlayerRelevancyBase* AssetPlayer = GetRelevantAssetPlayerInterfaceFromState(MachineIndex, StateIndex);
    return AssetPlayer && AssetPlayer->GetAnimAsset() ? AssetPlayer->GetCurrentAssetLength() : 0.0f;
}

float FAnimInstanceProxy::GetRelevantAnimTime(int32 MachineIndex, int32 StateIndex) const {
    const FAnimNode_AssetPlayerRelevancyBase* AssetPlayer = GetRelevantAssetPlayerInterfaceFromState(MachineIndex, StateIndex);
    return AssetPlayer ? AssetPlayer->GetCurrentAssetTimePlayRateAdjusted() : 0.0f;
}

float FAnimInstanceProxy::GetRelevantAnimTimeFraction(int32 MachineIndex, int32 StateIndex) const {
    const FAnimNode_AssetPlayerRelevancyBase* AssetPlayer = GetRelevantAssetPlayerInterfaceFromState(MachineIndex, StateIndex);
    if (AssetPlayer == nullptr || AssetPlayer->GetCurrentAssetLength() <= 0.0f) return 0.0f;
    return AssetPlayer->GetCurrentAssetTimePlayRateAdjusted() / AssetPlayer->GetCurrentAssetLength();
}

float FAnimInstanceProxy::GetInstanceMachineWeight(int32 MachineIndex) const {
    const FAnimNode_StateMachine* StateMachine = GetStateMachineInstance(MachineIndex);
    return StateMachine ? MachineWeightArrays[1 - BufferWriteIndex][StateMachine->StateMachineIndexInClass] : 0.0f;
}

float FAnimInstanceProxy::GetInstanceStateWeight(int32 MachineIndex, int32 StateIndex) const {
    const FAnimNode_StateMachine* StateMachine = GetStateMachineInstance(MachineIndex);
    if (StateMachine == nullptr) return 0.0f;
    return StateWeightArrays[1 - BufferWriteIndex][StateMachineWeightOffsets[StateMachine->StateMachineIndexInClass] + StateIndex];
}

float FAnimInstanceProxy::GetInstanceCurrentStateElapsedTime(int32 MachineIndex) const {
    const FAnimNode_StateMachine* StateMachine = GetStateMachineInstance(MachineIndex);
    return StateMachine ? StateMachine->GetCurrentStateElapsedTime() : 0.0f;
}

void FAnimInstanceProxy::CacheBones() { CacheBones_WithRoot(RootNode); }

void FAnimInstanceProxy::ProcessCopies(int32 CallSite) {
    if (AnimClassInterface == nullptr || AnimClassInterface->GetPropertyAccessLibrary() == nullptr) return;
    AnimClassInterface->GetPropertyAccessLibrary()->ProcessCopies(AnimInstanceObject, CallSite);
}

void FAnimInstanceProxy::UpdateAnimation() {
    if (bDeferRootNodeInitialization) {
        bDeferRootNodeInitialization = false;
        InitializeRootNode();
    }
    CacheBones();
    ThreadSafeUpdate();
    UpdateAnimationNode(FAnimationUpdateContext(this, CurrentDeltaSeconds));
    Sync.TickAssetPlayerInstances(NotifyQueue, CurrentDeltaSeconds);
}

void FAnimInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext) {
    if (RootNode == nullptr) return;
    UpdateCounter.Increment();
    RootNode->Update_AnyThread(InContext);
    // The cached poses update after the graph, once, with the largest weight they were used with.
    for (FAnimNode_SaveCachedPose* PoseNode : SavedPoseQueue) PoseNode->PostGraphUpdate();
}

void FAnimInstanceProxy::EvaluateAnimation(FPoseContext& Output) {
    CacheBones();
    if (!Evaluate(Output)) Output.ResetToRefPose();
}

bool FAnimInstanceProxy::Evaluate(FPoseContext& Output) {
    if (RootNode == nullptr) return false;
    EvaluationCounter.Increment();
    RootNode->Evaluate_AnyThread(Output);
    return true;
}

FAnimNode_Base* FAnimInstanceProxy::GetMutableNodeFromIndexUntyped(int32 NodeIdx, const UScriptStruct* RequiredStructType) const {
    if (AnimClassInterface == nullptr) return nullptr;
    const TArray<FStructProperty*>& AnimNodeProperties = AnimClassInterface->GetAnimNodeProperties();
    const int32 InstanceIdx = AnimNodeProperties.Num() - 1 - NodeIdx;
    if (!AnimNodeProperties.IsValidIndex(InstanceIdx) || AnimClassInterface->GetLinkTarget(InstanceIdx) != InstanceIdx) return nullptr;
    const FStructProperty* NodeProperty = AnimNodeProperties[InstanceIdx];
    if (!NodeProperty->Struct->IsChildOf(RequiredStructType)) return nullptr;
    return NodeProperty->ContainerPtrToValuePtr<FAnimNode_Base>(AnimInstanceObject);
}

}
