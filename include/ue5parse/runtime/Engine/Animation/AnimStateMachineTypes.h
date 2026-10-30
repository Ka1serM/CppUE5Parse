#pragma once

#include "ue5parse/runtime/Engine/AlphaBlend.h"
#include "ue5parse/runtime/Engine/Animation/BlendProfile.h"

#include "AnimStateMachineTypes.generated.h"

namespace ue5parse::runtime {

namespace ETransitionLogicType {
enum Type : int32 { StandardBlend, Inertialization, Custom };
}

USTRUCT()
struct FAnimationStateBase {
    GENERATED_BODY()
    UPROPERTY()
    FName StateName;
};

USTRUCT()
struct FAnimationTransitionBetweenStates : public FAnimationStateBase {
    GENERATED_BODY()
    UPROPERTY()
    int32 PreviousState = INDEX_NONE;
    UPROPERTY()
    int32 NextState = INDEX_NONE;
    UPROPERTY()
    float CrossfadeDuration = 0;
    UPROPERTY()
    int32 StartNotify = INDEX_NONE;
    UPROPERTY()
    int32 EndNotify = INDEX_NONE;
    UPROPERTY()
    int32 InterruptNotify = INDEX_NONE;
    UPROPERTY()
    EAlphaBlendOption BlendMode = EAlphaBlendOption::CubicInOut;
    UPROPERTY()
    UCurveFloat* CustomCurve = nullptr;
    UPROPERTY()
    UBlendProfile* BlendProfile = nullptr;
    UPROPERTY()
    TEnumAsByte<ETransitionLogicType::Type> LogicType = ETransitionLogicType::StandardBlend;
    // VALORANT's cooked UE 5.3 state-machine extension.
    UPROPERTY()
    bool bTransitionDisabled = false;
};

USTRUCT()
struct FBakedStateExitTransition {
    GENERATED_BODY()
    UPROPERTY()
    int32 CanTakeDelegateIndex = INDEX_NONE;
    UPROPERTY()
    int32 CustomResultNodeIndex = INDEX_NONE;
    UPROPERTY()
    int32 TransitionIndex = INDEX_NONE;
    UPROPERTY()
    bool bDesiredTransitionReturnValue = true;
    UPROPERTY()
    bool bAutomaticRemainingTimeRule = false;
    UPROPERTY()
    float AutomaticRuleTriggerTime = -1;
    UPROPERTY()
    FName SyncGroupNameToRequireValidMarkersRule;
    UPROPERTY()
    TArray<int32> PoseEvaluatorLinks;
};

USTRUCT()
struct FBakedAnimationState {
    GENERATED_BODY()
    UPROPERTY()
    FName StateName;
    UPROPERTY()
    TArray<FBakedStateExitTransition> Transitions;
    UPROPERTY()
    int32 StateRootNodeIndex = INDEX_NONE;
    UPROPERTY()
    int32 StartNotify = INDEX_NONE;
    UPROPERTY()
    int32 EndNotify = INDEX_NONE;
    UPROPERTY()
    int32 FullyBlendedNotify = INDEX_NONE;
    // VALORANT's cooked UE 5.3 state-machine extension.
    UPROPERTY()
    int32 FullyLeftNotify = INDEX_NONE;
    UPROPERTY()
    bool bResetStateOnEnter = false;
    UPROPERTY()
    bool bIsAConduit = false;
    UPROPERTY()
    bool bAnimStateDisabled = false;
    UPROPERTY()
    int32 EntryRuleNodeIndex = INDEX_NONE;
    UPROPERTY()
    TArray<int32> PlayerNodeIndices;
    UPROPERTY()
    TArray<int32> LayerNodeIndices;
    UPROPERTY()
    bool bAlwaysResetOnEntry = false;
};

USTRUCT()
struct FBakedAnimationStateMachine {
    GENERATED_BODY()
    UPROPERTY()
    FName MachineName;
    UPROPERTY()
    int32 InitialState = INDEX_NONE;
    UPROPERTY()
    TArray<FBakedAnimationState> States;
    UPROPERTY()
    TArray<FAnimationTransitionBetweenStates> Transitions;

    int32 FindStateIndex(FName Name) const;
    int32 FindTransitionIndex(int32 From, int32 To) const;
};

}
