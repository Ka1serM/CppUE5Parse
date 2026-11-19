#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

#include "GameplayEffectTypes.generated.h"

namespace ue5parse::runtime {

class AActor;
class UAbilitySystemComponent;
class UPhysicalMaterial;
class USceneComponent;

USTRUCT()
struct FGameplayEffectContextHandle {
    GENERATED_BODY()
};

USTRUCT()
struct FGameplayCueParameters {
    GENERATED_BODY()
    UPROPERTY()
    float NormalizedMagnitude = 0;
    UPROPERTY()
    float RawMagnitude = 0;
    UPROPERTY()
    FGameplayEffectContextHandle EffectContext;
    UPROPERTY()
    mutable FGameplayTag MatchedTagName;
    UPROPERTY()
    mutable FGameplayTag OriginalTag;
    UPROPERTY()
    FGameplayTagContainer AggregatedSourceTags;
    UPROPERTY()
    FGameplayTagContainer AggregatedTargetTags;
    UPROPERTY()
    FVector_NetQuantize10 Location;
    UPROPERTY()
    FVector_NetQuantizeNormal Normal;
    UPROPERTY()
    TWeakObjectPtr<AActor> Instigator;
    UPROPERTY()
    TWeakObjectPtr<AActor> EffectCauser;
    UPROPERTY()
    TWeakObjectPtr<const UObject> SourceObject;
    UPROPERTY()
    TWeakObjectPtr<const UPhysicalMaterial> PhysicalMaterial;
    UPROPERTY()
    int32 GameplayEffectLevel = 1;
    UPROPERTY()
    int32 AbilityLevel = 1;
    UPROPERTY()
    TWeakObjectPtr<USceneComponent> TargetAttachComponent;
    UPROPERTY()
    bool bReplicateLocationWhenUsingMinimalRepProxy = false;
};

USTRUCT()
struct FTagContainerAggregator {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayTagContainer CapturedActorTags;
    UPROPERTY()
    FGameplayTagContainer CapturedSpecTags;
    UPROPERTY()
    FGameplayTagContainer ScopedTags;
};

USTRUCT()
struct FGameplayTagCountContainer {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> Owner = nullptr;
};

USTRUCT()
struct FMinimalReplicationTagCountMap {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> Owner = nullptr;
};

}
