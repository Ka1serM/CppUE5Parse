#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"

#include "NetworkPhysicsSettingsComponent.generated.h"

namespace ue5parse::runtime {

class UNetworkPhysicsSettingsDataAsset;

USTRUCT()
struct FNetworkPhysicsSettingsNetworkPhysicsComponent {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bOverrideRedundantInputs : 1 = false;
    UPROPERTY()
    uint16 RedundantInputs = 0;
    UPROPERTY()
    uint32 bOverrideRedundantStates : 1 = false;
    UPROPERTY()
    uint16 RedundantStates = 0;
    UPROPERTY()
    uint32 bOverrideCompareStateToTriggerRewind : 1 = false;
    UPROPERTY()
    bool bCompareStateToTriggerRewind = false;
    UPROPERTY()
    uint32 bOverrideCompareInputToTriggerRewind : 1 = false;
    UPROPERTY()
    bool bCompareInputToTriggerRewind = false;
    UPROPERTY()
    uint32 bOverrideEnableUnreliableFlow : 1 = false;
    UPROPERTY()
    bool bEnableUnreliableFlow = false;
    UPROPERTY()
    uint32 bOverrideEnableReliableFlow : 1 = false;
    UPROPERTY()
    bool bEnableReliableFlow = false;
    UPROPERTY()
    uint32 bOverrideApplyDataInsteadOfMergeData : 1 = false;
    UPROPERTY()
    bool bApplyDataInsteadOfMergeData = false;
    UPROPERTY()
    uint32 bOverrideAllowInputExtrapolation : 1 = false;
    UPROPERTY()
    bool bAllowInputExtrapolation = false;
    UPROPERTY()
    uint32 bOverrideValidateDataOnGameThread : 1 = false;
    UPROPERTY()
    bool bValidateDataOnGameThread = false;
    UPROPERTY()
    uint32 bOverrideRedundantRemoteInputs : 1 = false;
    UPROPERTY()
    uint16 RedundantRemoteInputs = 0;
    UPROPERTY()
    uint32 bOverridebCompareStateToTriggerRewindIncludeSimProxies : 1 = false;
    UPROPERTY()
    bool bCompareStateToTriggerRewindIncludeSimProxies = false;
    UPROPERTY()
    uint32 bOverrideApplySimProxyStateAtRuntime : 1 = false;
    UPROPERTY()
    bool bApplySimProxyStateAtRuntime = false;
    UPROPERTY()
    uint32 bOverrideApplySimProxyInputAtRuntime : 1 = false;
    UPROPERTY()
    bool bApplySimProxyInputAtRuntime = false;
    UPROPERTY()
    FRuntimeFloatCurve InputDecayCurve;
    UPROPERTY()
    uint32 bOverrideTriggerResimOnInputReceive : 1 = false;
    UPROPERTY()
    uint32 bOverrideApplyInputDecayOverSetTime : 1 = false;
    UPROPERTY()
    uint32 bOverrideInputDecaySetTime : 1 = false;
    UPROPERTY()
    bool bTriggerResimOnInputReceive = false;
    UPROPERTY()
    bool bApplyInputDecayOverSetTime = false;
    UPROPERTY()
    float InputDecaySetTime = 0.0f;
    UPROPERTY()
    uint32 bOverrideDynamicInputBufferScalingEnabled : 1 = false;
    UPROPERTY()
    uint32 bOverrideApplyInputDecaySimProxyInputAtRuntime : 1 = false;
    UPROPERTY()
    uint32 bOverrideInputDecaySimProxyInputAtRuntime : 1 = false;
    UPROPERTY()
    uint32 bOverrideEnableLagScalingSimProxyRuntimeInputDecay : 1 = false;
    UPROPERTY()
    uint32 bOverrideEnableLagScalingInputDecay : 1 = false;
    UPROPERTY()
    uint32 bOverrideInputDecayReferenceLagMs : 1 = false;
    UPROPERTY()
    bool bDynamicInputBufferScalingEnabled = false;
    UPROPERTY()
    bool bApplyInputDecaySimProxyInputAtRuntime = false;
    UPROPERTY()
    float InputDecaySimProxyInputAtRuntime = 0.0f;
    UPROPERTY()
    bool bEnableLagScalingSimProxyRuntimeInputDecay = false;
    UPROPERTY()
    bool bEnableLagScalingInputDecay = false;
    UPROPERTY()
    float InputDecayReferenceLagMs = 0.0f;
};

UCLASS()
class UNetworkPhysicsSettingsComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UNetworkPhysicsSettingsComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FNetworkPhysicsSettingsNetworkPhysicsComponent NetworkPhysicsComponentSettings;
    UPROPERTY()
    TObjectPtr<UNetworkPhysicsSettingsDataAsset> SettingsDataAsset = nullptr;
};

}
