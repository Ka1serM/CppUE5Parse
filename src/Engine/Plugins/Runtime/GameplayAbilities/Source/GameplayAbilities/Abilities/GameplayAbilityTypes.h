#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayEffectTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayPrediction.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

#include "GameplayAbilityTypes.generated.h"

namespace ue5parse::runtime {

class UAnimMontage;
class UGameplayAbility;

namespace EGameplayAbilityInstancingPolicy {

UENUM()
enum Type : int32 { NonInstanced, InstancedPerActor, InstancedPerExecution };

}

namespace EGameplayAbilityNetExecutionPolicy {

UENUM()
enum Type : int32 { LocalPredicted, LocalOnly, ServerInitiated, ServerOnly };

}

namespace EGameplayAbilityNetSecurityPolicy {

UENUM()
enum Type : int32 { ClientOrServer, ServerOnlyExecution, ServerOnlyTermination, ServerOnly };

}

namespace EGameplayAbilityReplicationPolicy {

UENUM()
enum Type : int32 { ReplicateNo, ReplicateYes };

}

namespace EGameplayAbilityActivationMode {

UENUM()
enum Type : int32 { Authority, NonAuthority, Predicting, Confirmed, Rejected };

}

USTRUCT()
struct FGameplayAbilityLocalAnimMontage {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UAnimMontage> AnimMontage = nullptr;
    UPROPERTY()
    uint8 PlayInstanceId = 0;
    UPROPERTY()
    bool PlayBit = false;
    UPROPERTY()
    FPredictionKey PredictionKey;
    UPROPERTY()
    TWeakObjectPtr<UGameplayAbility> AnimatingAbility;
};

USTRUCT()
struct FGameplayEventData {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayTag EventTag;
    UPROPERTY()
    TObjectPtr<const AActor> Instigator = nullptr;
    UPROPERTY()
    TObjectPtr<const AActor> Target = nullptr;
    UPROPERTY()
    TObjectPtr<const UObject> OptionalObject = nullptr;
    UPROPERTY()
    TObjectPtr<const UObject> OptionalObject2 = nullptr;
    UPROPERTY()
    FGameplayEffectContextHandle ContextHandle;
    UPROPERTY()
    FGameplayTagContainer InstigatorTags;
    UPROPERTY()
    FGameplayTagContainer TargetTags;
    UPROPERTY()
    float EventMagnitude = 0;
    UPROPERTY()
    FGameplayAbilityTargetDataHandle TargetData;
    UPROPERTY()
    FInstancedStruct InstancedEventData;
    UPROPERTY()
    float Var1 = 0;
    UPROPERTY()
    float Var2 = 0;
    UPROPERTY()
    float Var3 = 0;
};

}
