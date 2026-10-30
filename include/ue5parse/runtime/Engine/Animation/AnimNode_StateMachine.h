#pragma once

#include <memory>
#include <vector>

#include "ue5parse/runtime/Engine/AlphaBlend.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimStateMachineTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"

#include "AnimNode_StateMachine.generated.h"

namespace ue5parse::runtime {

struct FAnimNode_AssetPlayerRelevancyBase;

// A transition that blends from one state to the next (UE's FAnimationActiveTransitionEntry).
struct FAnimationActiveTransitionEntry {
    float ElapsedTime = 0;
    float Alpha = 0;
    float CrossfadeDuration = 0;
    FAlphaBlend Blend;
    int32 NextState = INDEX_NONE;
    int32 PreviousState = INDEX_NONE;
    TEnumAsByte<ETransitionLogicType::Type> LogicType = ETransitionLogicType::StandardBlend;
    // The transition's notifies in the class's notify table.
    int32 StartNotify = INDEX_NONE;
    int32 EndNotify = INDEX_NONE;
    int32 InterruptNotify = INDEX_NONE;
    bool bActive = false;

    FAnimationActiveTransitionEntry() = default;
    // A transition that starts with the next state at its existing weight, and CrossfadeTimeAdjustment seconds of its
    // crossfade already past (an automatic rule that fired late).
    FAnimationActiveTransitionEntry(int32 NextStateID, float ExistingWeightOfNextState, int32 PreviousStateID,
                                    const FAnimationTransitionBetweenStates& ReferenceTransitionInfo, float CrossfadeTimeAdjustment);
    void Update(const FAnimationUpdateContext& Context, bool& bOutFinished);

private:
    // The fraction of the blend that leaves the alpha at InFraction.
    static float CalculateInverseAlpha(EAlphaBlendOption BlendMode, float InFraction);
};

// A transition that a state's rules allow (UE's FAnimationPotentialTransition).
struct FAnimationPotentialTransition {
    int32 TargetState = INDEX_NONE;
    float CrossfadeTimeAdjustment = 0;
    const FBakedStateExitTransition* TransitionRule = nullptr;

    bool IsValid() const { return TargetState != INDEX_NONE && TransitionRule != nullptr && TransitionRule->TransitionIndex != INDEX_NONE; }
};

// A state machine of the class (UE's Animation/AnimNode_StateMachine.h): each update takes the transitions that the
// current state's rules allow, then updates the states that blend; the evaluation blends their poses.
USTRUCT()
struct FAnimNode_StateMachine : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 StateMachineIndexInClass = 0;
    UPROPERTY()
    int32 MaxTransitionsPerFrame = 3;
    UPROPERTY()
    int32 MaxTransitionsRequests = 32;
    UPROPERTY()
    bool bSkipFirstUpdateTransition = true;
    UPROPERTY()
    bool bReinitializeOnBecomingRelevant = true;
    UPROPERTY()
    bool bCreateNotifyMetaData = true;
    UPROPERTY()
    bool bAllowConduitEntryStates = false;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    // The baked machine of the class, set when the instance links its nodes.
    void CacheMachineDescription(const class UAnimBlueprintGeneratedClass* AnimBlueprintClass);
    const FBakedAnimationStateMachine* GetMachineDescription() const { return MachineDescription; }
    int32 GetCurrentState() const { return CurrentState; }
    float GetCurrentStateElapsedTime() const { return ElapsedTime; }
    float GetStateWeight(int32 StateIndex) const;
    const FBakedAnimationState& GetStateInfo(int32 StateIndex) const { return MachineDescription->States[StateIndex]; }
    const FAnimationTransitionBetweenStates& GetTransitionInfo(int32 TransIndex) const { return MachineDescription->Transitions[TransIndex]; }
    float GetRelevantAnimTimeRemaining(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const;
    float GetRelevantAnimTimeRemainingFraction(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const;
    // The player of the state with the largest weight that takes part in relevancy tests, or null.
    const FAnimNode_AssetPlayerRelevancyBase* GetRelevantAssetPlayerInterfaceFromState(const FAnimInstanceProxy* InAnimInstanceProxy,
                                                                                      const FBakedAnimationState& StateInfo) const;

private:
    const FBakedAnimationState& GetStateInfo() const { return MachineDescription->States[CurrentState]; }
    bool IsAConduitState(int32 StateIndex) const { return MachineDescription->States.IsValidIndex(StateIndex) && GetStateInfo(StateIndex).bIsAConduit; }
    void SetState(const FAnimationBaseContext& Context, int32 NewStateIndex);
    void TransitionToState(const FAnimationUpdateContext& Context, const FAnimationTransitionBetweenStates& TransitionInfo,
                           const FAnimationPotentialTransition& BakedTransitionInfo);
    bool FindValidTransition(const FAnimationUpdateContext& Context, int32 StateIndex, FAnimationPotentialTransition& OutPotentialTransition,
                             TArray<int32>& OutVisitedStateIndices);
    void UpdateTransitionStates(const FAnimationUpdateContext& Context, FAnimationActiveTransitionEntry& Transition);
    void UpdateState(int32 StateIndex, const FAnimationUpdateContext& Context);
    const FPoseContext& EvaluateState(int32 StateIndex, const FPoseContext& Context);
    void EvaluateTransitionStandardBlend(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition, bool bIntermediatePoseIsValid);
    void ConditionallyCacheBonesForState(int32 StateIndex, const FAnimationBaseContext& Context);

    const FBakedAnimationStateMachine* MachineDescription = nullptr;
    TArray<FPoseLink> StatePoseLinks;
    TArray<FAnimationActiveTransitionEntry> ActiveTransitionArray;
    TArray<int32> StatesUpdated;
    TArray<FGraphTraversalCounter> StateCacheBoneCounters;
    // During an evaluation: the poses of the states it evaluated, made on first use. The evaluation owns them.
    std::vector<std::unique_ptr<FPoseContext>>* StateCachedPoses = nullptr;
    FGraphTraversalCounter UpdateCounter;
    int32 CurrentState = INDEX_NONE;
    float ElapsedTime = 0;
    bool bFirstUpdate = true;
};

}
