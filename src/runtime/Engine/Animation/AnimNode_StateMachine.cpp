#include "ue5parse/runtime/Engine/Animation/AnimNode_StateMachine.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerRelevancyBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_TransitionResult.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

// UE's Animation/AnimNode_StateMachine.cpp (UE 5.3) with standard blends. The state notifies, the anim node functions
// of the states, the transition events and the custom transition graphs are not part of the runtime; an
// inertialization transition switches at once, as in UE without an inertialization node.

namespace ue5parse::runtime {

FAnimationActiveTransitionEntry::FAnimationActiveTransitionEntry(int32 NextStateID, float ExistingWeightOfNextState, int32 PreviousStateID,
                                                                 const FAnimationTransitionBetweenStates& ReferenceTransitionInfo,
                                                                 float CrossfadeTimeAdjustment)
    : NextState(NextStateID), PreviousState(PreviousStateID), LogicType(ReferenceTransitionInfo.LogicType), StartNotify(ReferenceTransitionInfo.StartNotify),
      EndNotify(ReferenceTransitionInfo.EndNotify), InterruptNotify(ReferenceTransitionInfo.InterruptNotify), bActive(true) {
    const EAlphaBlendOption BlendOption = ReferenceTransitionInfo.BlendMode;
    CrossfadeDuration = LogicType == ETransitionLogicType::Inertialization
                            ? 0.0f
                            : FMath::Max(ReferenceTransitionInfo.CrossfadeDuration - CrossfadeTimeAdjustment, 0.0f) *
                                  CalculateInverseAlpha(BlendOption, 1.0f - ExistingWeightOfNextState);
    Blend.SetBlendTime(CrossfadeDuration);
    Blend.SetBlendOption(BlendOption);
    Blend.SetCustomCurve(ReferenceTransitionInfo.CustomCurve);
    Blend.SetValueRange(0.0f, 1.0f);
}

float FAnimationActiveTransitionEntry::CalculateInverseAlpha(EAlphaBlendOption BlendMode, float InFraction) {
    if (BlendMode != EAlphaBlendOption::HermiteCubic) return FMath::Clamp(InFraction, 0.0f, 1.0f);
    const float T = InFraction;
    return T * T * T * (4.0f / 3.0f) + T * T * -2.0f + T * (5.0f / 3.0f);
}

void FAnimationActiveTransitionEntry::Update(const FAnimationUpdateContext& Context, bool& bOutFinished) {
    bOutFinished = false;
    if (!bActive) return;
    ElapsedTime += Context.GetDeltaTime();
    Blend.Update(Context.GetDeltaTime());
    const float QueryAlpha = CrossfadeDuration > 0.0f ? ElapsedTime / CrossfadeDuration : 0.0f;
    Alpha = FAlphaBlend::AlphaToBlendOption(QueryAlpha, Blend.GetBlendOption(), Blend.GetCustomCurve());
    if (!Blend.IsComplete()) return;
    bActive = false;
    bOutFinished = true;
}

void FAnimNode_StateMachine::CacheMachineDescription(const UAnimBlueprintGeneratedClass* AnimBlueprintClass) {
    MachineDescription = AnimBlueprintClass->BakedStateMachines.IsValidIndex(StateMachineIndexInClass)
                             ? &AnimBlueprintClass->BakedStateMachines[StateMachineIndexInClass]
                             : nullptr;
}

void FAnimNode_StateMachine::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    if (MachineDescription == nullptr) return;
    ElapsedTime = 0.0f;
    CurrentState = INDEX_NONE;
    if (MachineDescription->States.Num() == 0) return;
    const FAnimInstanceProxy& Proxy = *Context.AnimInstanceProxy;
    const int32 NumNodes = Context.GetAnimClass()->GetAnimNodeProperties().Num();
    StatePoseLinks.Empty();
    StatePoseLinks.SetNum(MachineDescription->States.Num());
    for (int32 StateIndex = 0; StateIndex < MachineDescription->States.Num(); ++StateIndex) {
        const FBakedAnimationState& State = MachineDescription->States[StateIndex];
        if (State.StateRootNodeIndex != INDEX_NONE) StatePoseLinks[StateIndex].LinkID = NumNodes - 1 - State.StateRootNodeIndex;
        if (auto* EntryRule = Proxy.GetMutableNodeFromIndex<FAnimNode_TransitionResult>(State.EntryRuleNodeIndex))
            EntryRule->Initialize_AnyThread(Context);
        for (const FBakedStateExitTransition& Rule : State.Transitions)
            if (auto* Result = Proxy.GetMutableNodeFromIndex<FAnimNode_TransitionResult>(Rule.CanTakeDelegateIndex))
                Result->Initialize_AnyThread(Context);
    }
    StatesUpdated.Empty();
    ActiveTransitionArray.Empty();
    StateCacheBoneCounters.Empty();
    StateCacheBoneCounters.SetNum(MachineDescription->States.Num());
    SetState(Context, MachineDescription->InitialState);
    bFirstUpdate = true;
}

void FAnimNode_StateMachine::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    if (MachineDescription == nullptr) return;
    for (int32 StateIndex = 0; StateIndex < StatePoseLinks.Num(); ++StateIndex)
        if (GetStateWeight(StateIndex) > 0.0f) ConditionallyCacheBonesForState(StateIndex, Context);
}

void FAnimNode_StateMachine::ConditionallyCacheBonesForState(int32 StateIndex, const FAnimationBaseContext& Context) {
    if (StateCacheBoneCounters[StateIndex].IsSynchronized_Counter(Context.AnimInstanceProxy->GetCachedBonesCounter())) return;
    StateCacheBoneCounters[StateIndex].SynchronizeWith(Context.AnimInstanceProxy->GetCachedBonesCounter());
    StatePoseLinks[StateIndex].CacheBones(FAnimationCacheBonesContext(Context.AnimInstanceProxy));
}

void FAnimNode_StateMachine::Update_AnyThread(const FAnimationUpdateContext& Context) {
    FAnimInstanceProxy& Proxy = *Context.AnimInstanceProxy;
    Proxy.RecordMachineWeight(StateMachineIndexInClass, Context.GetFinalBlendWeight());
    // A machine that was not updated last frame starts again when it becomes relevant.
    if (!bFirstUpdate && bReinitializeOnBecomingRelevant && UpdateCounter.HasEverBeenUpdated() &&
        !UpdateCounter.WasSynchronizedCounter(Proxy.GetUpdateCounter()))
        Initialize_AnyThread(FAnimationInitializeContext(&Proxy));
    UpdateCounter.SynchronizeWith(Proxy.GetUpdateCounter());
    if (MachineDescription == nullptr || !MachineDescription->States.IsValidIndex(CurrentState)) return;

    // The first update enters the initial state.
    if (bFirstUpdate) Proxy.AddAnimNotifyFromGeneratedClass(GetStateInfo().StartNotify);

    int32 TransitionCountThisFrame = 0;
    bool bFoundValidTransition = false;
    do {
        bFoundValidTransition = false;
        FAnimationPotentialTransition PotentialTransition;
        TArray<int32> VisitedStateIndices;
        FindValidTransition(Context, CurrentState, PotentialTransition, VisitedStateIndices);
        if (PotentialTransition.IsValid() && PotentialTransition.TargetState != CurrentState) {
            bFoundValidTransition = true;
            TransitionToState(Context, GetTransitionInfo(PotentialTransition.TransitionRule->TransitionIndex), PotentialTransition);
            TransitionCountThisFrame++;
        }
    } while (bFoundValidTransition && TransitionCountThisFrame < MaxTransitionsPerFrame);

    if (bFirstUpdate) {
        if (bSkipFirstUpdateTransition) ActiveTransitionArray.Empty();
        bFirstUpdate = false;
    }

    StatesUpdated.Empty();
    if (ActiveTransitionArray.Num() > 0) {
        for (int32 Index = 0; Index < ActiveTransitionArray.Num(); ++Index) {
            FAnimationActiveTransitionEntry& Transition = ActiveTransitionArray[Index];
            bool bFinishedTrans = false;
            Transition.Update(Context, bFinishedTrans);
            if (!bFinishedTrans) {
                UpdateTransitionStates(Context, Transition);
                continue;
            }
            // The newest transition, which goes to the current state, ends and the state is fully blended in.
            if (Index == ActiveTransitionArray.Num() - 1) {
                Proxy.AddAnimNotifyFromGeneratedClass(Transition.EndNotify);
                Proxy.AddAnimNotifyFromGeneratedClass(GetStateInfo().FullyBlendedNotify);
            }
        }
        // A finished transition ends every transition before it.
        for (int32 Index = ActiveTransitionArray.Num() - 1; Index >= 0; --Index) {
            if (ActiveTransitionArray[Index].bActive) continue;
            ActiveTransitionArray.RemoveAt(0, Index + 1);
            break;
        }
    }
    if (ActiveTransitionArray.Num() == 0 && !IsAConduitState(CurrentState) && !StatesUpdated.Contains(CurrentState))
        StatePoseLinks[CurrentState].Update(Context);

    ElapsedTime += Context.GetDeltaTime();
    for (int32 StateIndex = 0; StateIndex < StatePoseLinks.Num(); ++StateIndex) {
        const float StateWeight = GetStateWeight(StateIndex);
        if (StateWeight > 0.0f) Proxy.RecordStateWeight(StateMachineIndexInClass, StateIndex, StateWeight);
    }
}

bool FAnimNode_StateMachine::FindValidTransition(const FAnimationUpdateContext& Context, int32 StateIndex,
                                                 FAnimationPotentialTransition& OutPotentialTransition, TArray<int32>& OutVisitedStateIndices) {
    if (OutVisitedStateIndices.Contains(StateIndex)) return false;
    OutVisitedStateIndices.Add(StateIndex);
    const FAnimInstanceProxy& Proxy = *Context.AnimInstanceProxy;
    const FBakedAnimationState& StateInfo = GetStateInfo(StateIndex);
    if (auto* EntryRule = Proxy.GetMutableNodeFromIndex<FAnimNode_TransitionResult>(StateInfo.EntryRuleNodeIndex)) {
        EntryRule->GetEvaluateGraphExposedInputs().Execute(Context);
        if (!EntryRule->bCanEnterTransition) return false;
    }
    for (const FBakedStateExitTransition& TransitionRule : StateInfo.Transitions) {
        if (TransitionRule.CanTakeDelegateIndex == INDEX_NONE) continue;
        const FAnimationTransitionBetweenStates& TransitionInfo = GetTransitionInfo(TransitionRule.TransitionIndex);
        if (TransitionInfo.bTransitionDisabled) continue;
        auto* ResultNode = Proxy.GetMutableNodeFromIndex<FAnimNode_TransitionResult>(TransitionRule.CanTakeDelegateIndex);
        if (ResultNode == nullptr) continue;
        float CrossfadeTimeAdjustment = 0.0f;
        if (!TransitionRule.SyncGroupNameToRequireValidMarkersRule.isNone()) {
            // Marker sync is not part of the runtime, so no sync group has valid markers.
            ResultNode->bCanEnterTransition = false;
        } else if (TransitionRule.bAutomaticRemainingTimeRule) {
            bool bCanEnterTransition = false;
            if (const FAnimNode_AssetPlayerRelevancyBase* RelevantPlayer = GetRelevantAssetPlayerInterfaceFromState(&Proxy, StateInfo)) {
                if (const UAnimationAsset* AnimAsset = RelevantPlayer->GetAnimAsset()) {
                    const float AnimTimeRemaining = AnimAsset->GetPlayLength() - RelevantPlayer->GetAccumulatedTime();
                    const float CrossfadeDuration = GetStateInfo(TransitionInfo.NextState).bIsAConduit ? 0.0f : TransitionInfo.CrossfadeDuration;
                    const float TriggerTime = TransitionRule.AutomaticRuleTriggerTime >= 0.0f ? TransitionRule.AutomaticRuleTriggerTime : CrossfadeDuration;
                    CrossfadeTimeAdjustment = TriggerTime - AnimTimeRemaining;
                    bCanEnterTransition = CrossfadeTimeAdjustment >= 0.0f;
                }
            }
            ResultNode->bCanEnterTransition = bCanEnterTransition;
        } else {
            ResultNode->GetEvaluateGraphExposedInputs().Execute(Context);
        }
        if (ResultNode->bCanEnterTransition != TransitionRule.bDesiredTransitionReturnValue) continue;
        const int32 NextState = TransitionInfo.NextState;
        if (GetStateInfo(NextState).bIsAConduit) {
            if (FindValidTransition(Context, NextState, OutPotentialTransition, OutVisitedStateIndices)) return true;
            continue;
        }
        OutPotentialTransition.TransitionRule = &TransitionRule;
        OutPotentialTransition.TargetState = NextState;
        OutPotentialTransition.CrossfadeTimeAdjustment = CrossfadeTimeAdjustment;
        return true;
    }
    return false;
}

void FAnimNode_StateMachine::UpdateTransitionStates(const FAnimationUpdateContext& Context, FAnimationActiveTransitionEntry& Transition) {
    if (!Transition.bActive) return;
    switch (Transition.LogicType) {
        case ETransitionLogicType::StandardBlend:
            UpdateState(Transition.PreviousState, Context.FractionalWeight(GetStateWeight(Transition.PreviousState)));
            UpdateState(Transition.NextState, Context.FractionalWeight(GetStateWeight(Transition.NextState)));
            break;
        case ETransitionLogicType::Inertialization: UpdateState(Transition.NextState, Context); break;
        default: ReportLoadProblem("anim.transition_custom", FString("a state machine takes a custom transition, which does not update")); break;
    }
}

void FAnimNode_StateMachine::UpdateState(int32 StateIndex, const FAnimationUpdateContext& Context) {
    if (StateIndex == INDEX_NONE || StatesUpdated.Contains(StateIndex) || IsAConduitState(StateIndex)) return;
    StatesUpdated.Add(StateIndex);
    StatePoseLinks[StateIndex].Update(Context);
}

void FAnimNode_StateMachine::SetState(const FAnimationBaseContext& Context, int32 NewStateIndex) {
    if (NewStateIndex == CurrentState) return;
    const bool bForceReset = MachineDescription->States.IsValidIndex(NewStateIndex) && MachineDescription->States[NewStateIndex].bAlwaysResetOnEntry;
    const bool bAlreadyActive = GetStateWeight(NewStateIndex) > 0.0f;
    CurrentState = FMath::Clamp(NewStateIndex, 0, MachineDescription->States.Num() - 1);
    ElapsedTime = 0.0f;
    for (int32 PlayerIndex : GetStateInfo().PlayerNodeIndices)
        if (auto* Player = Context.AnimInstanceProxy->GetMutableNodeFromIndex<FAnimNode_AssetPlayerRelevancyBase>(PlayerIndex))
            Player->ClearCachedBlendWeight();
    if ((bAlreadyActive && !bForceReset) || IsAConduitState(NewStateIndex)) return;
    StatePoseLinks[NewStateIndex].Initialize(FAnimationInitializeContext(Context.AnimInstanceProxy));
    if (Context.AnimInstanceProxy->GetCachedBonesCounter().HasEverBeenUpdated()) ConditionallyCacheBonesForState(NewStateIndex, Context);
}

void FAnimNode_StateMachine::TransitionToState(const FAnimationUpdateContext& Context, const FAnimationTransitionBetweenStates& TransitionInfo,
                                               const FAnimationPotentialTransition& BakedTransitionInfo) {
    // The transition in progress is interrupted.
    if (ActiveTransitionArray.Num() > 0 && ActiveTransitionArray.Last().bActive)
        Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(ActiveTransitionArray.Last().InterruptNotify);
    const int32 PreviousState = CurrentState;
    const int32 NextState = TransitionInfo.NextState;
    Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(GetStateInfo(PreviousState).EndNotify);
    Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(GetStateInfo(NextState).StartNotify);
    if (!IsAConduitState(PreviousState)) {
        if (TransitionInfo.LogicType == ETransitionLogicType::Inertialization)
            ReportLoadProblem("anim.inertialization", FString("a state machine requests an inertialization without an inertialization node"));
        ActiveTransitionArray.Emplace_GetRef(NextState, GetStateWeight(NextState), PreviousState, TransitionInfo,
                                             BakedTransitionInfo.CrossfadeTimeAdjustment);
        if (!bFirstUpdate || !bSkipFirstUpdateTransition) Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(TransitionInfo.StartNotify);
    }
    SetState(Context, NextState);
}

float FAnimNode_StateMachine::GetStateWeight(int32 StateIndex) const {
    if (ActiveTransitionArray.Num() == 0) return StateIndex == CurrentState ? 1.0f : 0.0f;
    float TotalWeight = 0.0f;
    for (int32 Index = 0; Index < ActiveTransitionArray.Num(); ++Index) {
        const FAnimationActiveTransitionEntry& Transition = ActiveTransitionArray[Index];
        const float SourceWeight = 1.0f - Transition.Alpha;
        if (Index > 0) TotalWeight *= SourceWeight;
        else if (Transition.PreviousState == StateIndex) TotalWeight += SourceWeight;
        if (Transition.NextState == StateIndex) TotalWeight += Transition.Alpha;
    }
    return FMath::Clamp(TotalWeight, 0.0f, 1.0f);
}

void FAnimNode_StateMachine::Evaluate_AnyThread(FPoseContext& Output) {
    if (MachineDescription == nullptr || IsAConduitState(CurrentState) || !MachineDescription->States.IsValidIndex(CurrentState)) {
        Output.Pose.ResetToRefPose();
        return;
    }
    if (ActiveTransitionArray.Num() == 0) {
        ConditionallyCacheBonesForState(CurrentState, Output);
        StatePoseLinks[CurrentState].Evaluate(Output);
        return;
    }
    std::vector<std::unique_ptr<FPoseContext>> CachedPoses(static_cast<std::size_t>(StatePoseLinks.Num()));
    StateCachedPoses = &CachedPoses;
    for (int32 Index = 0; Index < ActiveTransitionArray.Num(); ++Index) {
        FAnimationActiveTransitionEntry& ActiveTransition = ActiveTransitionArray[Index];
        if (!ActiveTransition.bActive) continue;
        if (ActiveTransition.LogicType == ETransitionLogicType::Inertialization) Output = EvaluateState(ActiveTransition.NextState, Output);
        else EvaluateTransitionStandardBlend(Output, ActiveTransition, Index > 0);
    }
    Output.Pose.NormalizeRotations();
    StateCachedPoses = nullptr;
}

void FAnimNode_StateMachine::EvaluateTransitionStandardBlend(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition,
                                                             bool bIntermediatePoseIsValid) {
    // A later transition blends from the result of the earlier ones.
    FPoseContext IntermediatePose(Output);
    if (bIntermediatePoseIsValid) IntermediatePose = Output;
    const FPoseContext& PreviousStateResult = bIntermediatePoseIsValid ? IntermediatePose : EvaluateState(Transition.PreviousState, Output);
    const FPoseContext& NextStateResult = EvaluateState(Transition.NextState, Output);
    const double PreviousWeight = 1.0 - Transition.Alpha;
    for (int32 Index = 0; Index < Output.Pose.GetNumBones(); ++Index) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        Output.Pose[BoneIndex] = PreviousStateResult.Pose[BoneIndex] * PreviousWeight;
        Output.Pose[BoneIndex].AccumulateWithShortestRotation(NextStateResult.Pose[BoneIndex], Transition.Alpha);
    }
    Output.Curve.Override(PreviousStateResult.Curve, 1.0f - Transition.Alpha);
    Output.Curve.Accumulate(NextStateResult.Curve, Transition.Alpha);
}

const FPoseContext& FAnimNode_StateMachine::EvaluateState(int32 StateIndex, const FPoseContext& Context) {
    std::unique_ptr<FPoseContext>& CachedPose = (*StateCachedPoses)[static_cast<std::size_t>(StateIndex)];
    if (CachedPose != nullptr) return *CachedPose;
    CachedPose = std::make_unique<FPoseContext>(Context);
    if (IsAConduitState(StateIndex)) {
        CachedPose->ResetToRefPose();
        return *CachedPose;
    }
    ConditionallyCacheBonesForState(StateIndex, Context);
    StatePoseLinks[StateIndex].Evaluate(*CachedPose);
    return *CachedPose;
}

float FAnimNode_StateMachine::GetRelevantAnimTimeRemaining(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const {
    const FAnimNode_AssetPlayerRelevancyBase* AssetPlayer = GetRelevantAssetPlayerInterfaceFromState(InAnimInstanceProxy, GetStateInfo(StateIndex));
    if (AssetPlayer == nullptr || AssetPlayer->GetAnimAsset() == nullptr) return UE_MAX_FLT;
    return AssetPlayer->GetCurrentAssetLength() - AssetPlayer->GetCurrentAssetTimePlayRateAdjusted();
}

float FAnimNode_StateMachine::GetRelevantAnimTimeRemainingFraction(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const {
    const FAnimNode_AssetPlayerRelevancyBase* AssetPlayer = GetRelevantAssetPlayerInterfaceFromState(InAnimInstanceProxy, GetStateInfo(StateIndex));
    if (AssetPlayer == nullptr || AssetPlayer->GetAnimAsset() == nullptr) return 1.0f;
    const float Length = AssetPlayer->GetCurrentAssetLength();
    return Length > 0.0f ? (Length - AssetPlayer->GetCurrentAssetTimePlayRateAdjusted()) / Length : 1.0f;
}

const FAnimNode_AssetPlayerRelevancyBase* FAnimNode_StateMachine::GetRelevantAssetPlayerInterfaceFromState(const FAnimInstanceProxy* InAnimInstanceProxy,
                                                                                                          const FBakedAnimationState& StateInfo) const {
    const FAnimNode_AssetPlayerRelevancyBase* ResultPlayer = nullptr;
    float MaxWeight = 0.0f;
    for (int32 PlayerIndex : StateInfo.PlayerNodeIndices) {
        const auto* Player = InAnimInstanceProxy->GetMutableNodeFromIndex<FAnimNode_AssetPlayerRelevancyBase>(PlayerIndex);
        if (Player == nullptr || Player->GetIgnoreForRelevancyTest() || Player->GetCachedBlendWeight() <= MaxWeight) continue;
        MaxWeight = Player->GetCachedBlendWeight();
        ResultPlayer = Player;
    }
    return ResultPlayer;
}

}
