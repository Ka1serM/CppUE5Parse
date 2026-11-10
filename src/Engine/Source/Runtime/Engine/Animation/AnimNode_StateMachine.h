#pragma once

#include <memory>
#include <vector>

#include "Engine/Source/Runtime/Engine/AlphaBlend.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimStateMachineTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"

#include "AnimNode_StateMachine.generated.h"

namespace ue5parse::runtime {

struct FAnimNode_AssetPlayerRelevancyBase;

struct FAnimationActiveTransitionEntry {
    float ElapsedTime = 0;
    float Alpha = 0;
    float CrossfadeDuration = 0;
    FAlphaBlend Blend;
    int32 NextState = INDEX_NONE;
    int32 PreviousState = INDEX_NONE;
    TEnumAsByte<ETransitionLogicType::Type> LogicType = ETransitionLogicType::StandardBlend;
    int32 StartNotify = INDEX_NONE;
    int32 EndNotify = INDEX_NONE;
    int32 InterruptNotify = INDEX_NONE;
    bool bActive = false;

    FAnimationActiveTransitionEntry() = default;
    FAnimationActiveTransitionEntry(int32 NextStateID, float ExistingWeightOfNextState, int32 PreviousStateID,
                                    const FAnimationTransitionBetweenStates& ReferenceTransitionInfo, float CrossfadeTimeAdjustment);
    void Update(const FAnimationUpdateContext& Context, bool& bOutFinished);

private:
    static float CalculateInverseAlpha(EAlphaBlendOption BlendMode, float InFraction);
};

struct FAnimationPotentialTransition {
    int32 TargetState = INDEX_NONE;
    float CrossfadeTimeAdjustment = 0;
    const FBakedStateExitTransition* TransitionRule = nullptr;

    bool IsValid() const { return TargetState != INDEX_NONE && TransitionRule != nullptr && TransitionRule->TransitionIndex != INDEX_NONE; }
};

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

    void CacheMachineDescription(const class UAnimBlueprintGeneratedClass* AnimBlueprintClass);
    int32 GetCurrentState() const { return CurrentState; }
    float GetCurrentStateElapsedTime() const { return ElapsedTime; }
    float GetStateWeight(int32 StateIndex) const;
    const FBakedAnimationState& GetStateInfo(int32 StateIndex) const { return MachineDescription->States[StateIndex]; }
    const FAnimationTransitionBetweenStates& GetTransitionInfo(int32 TransIndex) const { return MachineDescription->Transitions[TransIndex]; }
    float GetRelevantAnimTimeRemaining(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const;
    float GetRelevantAnimTimeRemainingFraction(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const;
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
    std::vector<std::unique_ptr<FPoseContext>>* StateCachedPoses = nullptr;
    FGraphTraversalCounter UpdateCounter;
    int32 CurrentState = INDEX_NONE;
    float ElapsedTime = 0;
    bool bFirstUpdate = true;
};

}
