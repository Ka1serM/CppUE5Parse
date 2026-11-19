#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Abilities/GameplayAbilityRepAnimMontage.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Abilities/GameplayAbilityTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/AttributeSet.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayAbilitySpec.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayCueInterface.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayEffect.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayEffectTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayPrediction.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/GameplayTasks/GameplayTasksNative.h"

#include "AbilitySystemComponent.generated.h"

namespace ue5parse::runtime {

class AGameplayAbilityTargetActor;
class UAbilityTask;
class UGameplayAbility;

USTRUCT()
struct FAttributeDefaults {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSubclassOf<UAttributeSet> Attributes;
    UPROPERTY()
    TObjectPtr<UDataTable> DefaultStartingTable = nullptr;
};

UCLASS()
class UAbilitySystemComponent : public UGameplayTasksComponent {
    GENERATED_BODY()
public:
    explicit UAbilitySystemComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<FAttributeDefaults> DefaultStartingData;
    UPROPERTY()
    FName AffectedAnimInstanceTag;
    UPROPERTY()
    float OutgoingDuration = 0;
    UPROPERTY()
    float IncomingDuration = 0;
    UPROPERTY()
    TArray<FString> ClientDebugStrings;
    UPROPERTY()
    TArray<FString> ServerDebugStrings;
    UPROPERTY()
    bool UserAbilityActivationInhibited = false;
    UPROPERTY()
    bool ReplicationProxyEnabled = false;
    UPROPERTY()
    bool bSuppressGrantAbility = false;
    UPROPERTY()
    bool bSuppressGameplayCues = false;
    UPROPERTY()
    TArray<TObjectPtr<AGameplayAbilityTargetActor>> SpawnedTargetActors;
    UPROPERTY()
    TObjectPtr<AActor> OwnerActor = nullptr;
    UPROPERTY()
    TObjectPtr<AActor> AvatarActor = nullptr;
    UPROPERTY(Transient)
    FGameplayAbilitySpecContainer ActivatableAbilities;
    UPROPERTY()
    TArray<TObjectPtr<UGameplayAbility>> AllReplicatedInstancedAbilities;
    UPROPERTY()
    FGameplayAbilityRepAnimMontage RepAnimMontageInfo;
    UPROPERTY()
    bool bCachedIsNetSimulated = false;
    UPROPERTY()
    bool bPendingMontageRep = false;
    UPROPERTY()
    FGameplayAbilityLocalAnimMontage LocalAnimMontageInfo;
    UPROPERTY()
    FActiveGameplayEffectsContainer ActiveGameplayEffects;
    UPROPERTY()
    FActiveGameplayCueContainer ActiveGameplayCues;
    UPROPERTY()
    FActiveGameplayCueContainer MinimalReplicationGameplayCues;
    UPROPERTY()
    FGameplayTagCountContainer GameplayTagCountContainer;
    UPROPERTY()
    FMinimalReplicationTagCountMap MinimalReplicationTags;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UAttributeSet>> SpawnedAttributes;
    UPROPERTY()
    FMinimalReplicationTagCountMap ReplicatedLooseTags;
    UPROPERTY(Transient)
    FReplicatedPredictionKeyMap ReplicatedPredictionKeyMap;
    UPROPERTY()
    FPredictionKey ReplicatedPredictionKey;
    UPROPERTY()
    TArray<TObjectPtr<UGameplayAbility>> NonReplicatedInstancedAbilities;
    UPROPERTY()
    TArray<TObjectPtr<UGameplayAbility>> ReplicatedInstancedAbilities;
    UPROPERTY()
    TObjectPtr<UGameplayAbility> AnimatingAbility = nullptr;
    UPROPERTY()
    TObjectPtr<UGameplayAbility> TargetingAbility = nullptr;
    UPROPERTY()
    TObjectPtr<AActor> AbilityActor = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UAbilityTask>> SimulatedTasks;

    void OnRegister() override;

    float GetNumericAttribute(const FGameplayAttribute& Attribute) const;
    bool HasAttributeSetForAttribute(const FGameplayAttribute& Attribute) const;
    const UAttributeSet* GetAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass) const;
    const UAttributeSet* GetOrCreateAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass);
};

}
