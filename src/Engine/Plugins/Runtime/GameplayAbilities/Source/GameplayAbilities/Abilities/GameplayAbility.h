#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Abilities/GameplayAbilityTriggerType.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Abilities/GameplayAbilityTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayAbilitySpec.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "GameplayAbility.generated.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UGameplayEffect;
class UGameplayTask;

UCLASS()
class UGameplayAbility : public UObject {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent)
    void K2_ActivateAbility();

    UPROPERTY()
    FGameplayTagContainer AbilityTags;
    UPROPERTY()
    bool bReplicateInputDirectly = false;
    UPROPERTY()
    bool RemoteInstanceEnded = false;
    UPROPERTY()
    TEnumAsByte<EGameplayAbilityReplicationPolicy::Type> ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    UPROPERTY()
    TEnumAsByte<EGameplayAbilityInstancingPolicy::Type> InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    UPROPERTY()
    bool bServerRespectsRemoteAbilityCancellation = true;
    UPROPERTY()
    bool bRetriggerInstancedAbility = false;
    UPROPERTY()
    FGameplayAbilityActivationInfo CurrentActivationInfo;
    UPROPERTY()
    FGameplayEventData CurrentEventData;
    UPROPERTY()
    TEnumAsByte<EGameplayAbilityNetExecutionPolicy::Type> NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    UPROPERTY()
    TEnumAsByte<EGameplayAbilityNetSecurityPolicy::Type> NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
    UPROPERTY()
    TObjectPtr<UGameplayEffect> CooldownGameplayEffect = nullptr;
    UPROPERTY()
    TObjectPtr<UGameplayEffect> CostGameplayEffect = nullptr;
    UPROPERTY()
    TSubclassOf<UGameplayEffect> CostGameplayEffectClass;
    UPROPERTY()
    TArray<FAbilityTriggerData> AbilityTriggers;
    UPROPERTY()
    TSubclassOf<UGameplayEffect> CooldownGameplayEffectClass;
    UPROPERTY()
    FGameplayTagQuery CancelAbilitiesMatchingTagQuery;
    UPROPERTY()
    FGameplayTagQuery ConstTagQuery;
    UPROPERTY()
    FGameplayTagContainer CancelAbilitiesWithTag;
    UPROPERTY()
    FGameplayTagContainer BlockAbilitiesWithTag;
    UPROPERTY()
    FGameplayTagContainer ActivationOwnedTags;
    UPROPERTY()
    FGameplayTagContainer ActivationRequiredTags;
    UPROPERTY()
    FGameplayTagContainer ActivationBlockedTags;
    UPROPERTY()
    FGameplayTagContainer SourceRequiredTags;
    UPROPERTY()
    FGameplayTagContainer SourceBlockedTags;
    UPROPERTY()
    FGameplayTagContainer TargetRequiredTags;
    UPROPERTY()
    FGameplayTagContainer TargetBlockedTags;
    UPROPERTY()
    TArray<TObjectPtr<UGameplayTask>> ActiveTasks;
    UPROPERTY()
    TObjectPtr<UAnimMontage> CurrentMontage = nullptr;
    UPROPERTY()
    bool bIsActive = false;
    UPROPERTY()
    bool bIsAbilityEnding = false;
    UPROPERTY()
    bool bIsCancelable = false;
    UPROPERTY()
    bool bIsBlockingOtherAbilities = false;
    UPROPERTY()
    bool bMarkPendingKillOnAbilityEnd = false;
};

}
