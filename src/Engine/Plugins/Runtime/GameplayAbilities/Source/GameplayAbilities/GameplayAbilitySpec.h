#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Abilities/GameplayAbilityTriggerType.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Abilities/GameplayAbilityTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/ActiveGameplayEffectHandle.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayAbilitySpecHandle.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayPrediction.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/ScalableFloat.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"

#include "GameplayAbilitySpec.generated.h"

namespace ue5parse::runtime {

class UAbilitySystemComponent;
class UGameplayAbility;

UENUM()
enum class EGameplayEffectGrantedAbilityRemovePolicy : uint8 { CancelAbilityImmediately, RemoveAbilityOnEnd, DoNothing };

USTRUCT()
struct FGameplayAbilitySpecDef {
    GENERATED_BODY()
    UPROPERTY()
    TSubclassOf<UGameplayAbility> Ability;
    UPROPERTY()
    int32 Level = 1;
    UPROPERTY()
    FScalableFloat LevelScalableFloat{1.f};
    UPROPERTY()
    int32 InputID = INDEX_NONE;
    UPROPERTY()
    EGameplayEffectGrantedAbilityRemovePolicy RemovalPolicy = EGameplayEffectGrantedAbilityRemovePolicy::CancelAbilityImmediately;
    UPROPERTY()
    TWeakObjectPtr<UObject> SourceObject;
    UPROPERTY()
    FGameplayAbilitySpecHandle AssignedHandle;
};

USTRUCT()
struct FGameplayAbilityActivationInfo {
    GENERATED_BODY()
    UPROPERTY()
    mutable TEnumAsByte<EGameplayAbilityActivationMode::Type> ActivationMode = EGameplayAbilityActivationMode::Authority;
    UPROPERTY()
    uint8 bCanBeEndedByOtherInstance : 1 = 0;
    UPROPERTY()
    FPredictionKey PredictionKeyWhenActivated;
    UPROPERTY()
    mutable FPredictionKey PredictionKey;
};

USTRUCT()
struct FGameplayAbilitySpec : public FFastArraySerializerItem {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayAbilitySpecHandle Handle;
    UPROPERTY()
    TObjectPtr<UGameplayAbility> Ability = nullptr;
    UPROPERTY()
    int32 Level = 1;
    UPROPERTY()
    int32 InputID = INDEX_NONE;
    UPROPERTY()
    TWeakObjectPtr<UObject> SourceObject;
    UPROPERTY()
    uint8 ActiveCount = 0;
    UPROPERTY()
    uint8 InputPressed : 1 = 0;
    UPROPERTY()
    uint8 RemoveAfterActivation : 1 = 0;
    UPROPERTY()
    uint8 PendingRemove : 1 = 0;
    UPROPERTY()
    uint8 bActivateOnce : 1 = 0;
    UPROPERTY()
    FGameplayAbilityActivationInfo ActivationInfo;
    UPROPERTY()
    FGameplayTagContainer DynamicAbilityTags;
    UPROPERTY()
    TArray<FAbilityTriggerData> DynamicAbilityTriggers;
    UPROPERTY()
    TArray<TObjectPtr<UGameplayAbility>> NonReplicatedInstances;
    UPROPERTY()
    TArray<TObjectPtr<UGameplayAbility>> ReplicatedInstances;
    UPROPERTY()
    FActiveGameplayEffectHandle GameplayEffectHandle;
    UPROPERTY()
    bool IsActive = false;
};

USTRUCT()
struct FGameplayAbilitySpecContainer : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FGameplayAbilitySpec> Items;
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> Owner = nullptr;
};

}
