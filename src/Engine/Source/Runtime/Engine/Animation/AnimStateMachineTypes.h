#pragma once

#include "Engine/Source/Runtime/Engine/AlphaBlend.h"
#include "Engine/Source/Runtime/Engine/Animation/BlendProfile.h"

#include "AnimStateMachineTypes.generated.h"

namespace ue5parse::runtime {

namespace ETransitionBlendMode {

UENUM()
enum Type : int32 { TBM_Linear, TBM_Cubic };

}

namespace ETransitionLogicType {

UENUM()
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
    float MinTimeBeforeReentry = -1;
    UPROPERTY()
    int32 StartNotify = INDEX_NONE;
    UPROPERTY()
    int32 EndNotify = INDEX_NONE;
    UPROPERTY()
    int32 InterruptNotify = INDEX_NONE;
    UPROPERTY()
    EAlphaBlendOption BlendMode = EAlphaBlendOption::CubicInOut;
    UPROPERTY()
    TObjectPtr<UCurveFloat> CustomCurve = nullptr;
    UPROPERTY()
    TObjectPtr<UBlendProfile> BlendProfile = nullptr;
    UPROPERTY()
    TEnumAsByte<ETransitionBlendMode::Type> CrossfadeMode_DEPRECATED = ETransitionBlendMode::TBM_Cubic;
    UPROPERTY()
    TEnumAsByte<ETransitionLogicType::Type> LogicType = ETransitionLogicType::StandardBlend;
    UPROPERTY()
    bool bAllowInertializationForSelfTransitions = false;
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
    bool bOnlyEvaluateWhenActive = false;
    UPROPERTY()
    float AutomaticRuleTriggerTime = -1;
    UPROPERTY()
    FName SyncGroupNameToRequireValidMarkersRule;
    UPROPERTY()
    TArray<int32> PoseEvaluatorLinks;
    UPROPERTY()
    int32 StateSequencePlayerToQueryIndex = INDEX_NONE;
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
};

}
