#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/ActiveGameplayEffectHandle.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayAbilitySpec.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayEffectAttributeCaptureDefinition.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayEffectTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayPrediction.h"

#include "GameplayEffect.generated.h"

namespace ue5parse::runtime {

class UGameplayEffect;

USTRUCT()
struct FModifierSpec {
    GENERATED_BODY()
    UPROPERTY()
    float EvaluatedMagnitude = 0;
};

USTRUCT()
struct FGameplayEffectModifiedAttribute {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayAttribute Attribute;
    UPROPERTY()
    float TotalMagnitude = 0;
};

USTRUCT()
struct FGameplayEffectAttributeCaptureSpec {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayEffectAttributeCaptureDefinition BackingDefinition;
};

USTRUCT()
struct FGameplayEffectAttributeCaptureSpecContainer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FGameplayEffectAttributeCaptureSpec> SourceAttributes;
    UPROPERTY()
    TArray<FGameplayEffectAttributeCaptureSpec> TargetAttributes;
    UPROPERTY()
    bool bHasNonSnapshottedAttributes = false;
};

USTRUCT()
struct FGameplayEffectSpec {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<const UGameplayEffect> Def = nullptr;
    UPROPERTY()
    TArray<FGameplayEffectModifiedAttribute> ModifiedAttributes;
    UPROPERTY()
    FGameplayEffectAttributeCaptureSpecContainer CapturedRelevantAttributes;
    UPROPERTY()
    float Duration = 0;
    UPROPERTY()
    float Period = 0;
    UPROPERTY()
    float ChanceToApplyToTarget = 1;
    UPROPERTY()
    float ChanceToExecuteOnGameplayEffect = 0;
    UPROPERTY()
    bool bTopOfStack = false;
    UPROPERTY()
    FTagContainerAggregator CapturedSourceTags;
    UPROPERTY()
    FTagContainerAggregator CapturedTargetTags;
    UPROPERTY()
    FGameplayTagContainer DynamicGrantedTags;
    UPROPERTY()
    FGameplayTagContainer DynamicAssetTags;
    UPROPERTY()
    TArray<FModifierSpec> Modifiers;
    UPROPERTY()
    int32 StackCount = 1;
    UPROPERTY()
    uint8 bCompletedSourceAttributeCapture : 1 = 0;
    UPROPERTY()
    uint8 bCompletedTargetAttributeCapture : 1 = 0;
    UPROPERTY()
    uint8 bDurationLocked : 1 = 0;
    UPROPERTY()
    TArray<FGameplayAbilitySpecDef> GrantedAbilitySpecs;
    UPROPERTY()
    FGameplayEffectContextHandle EffectContext;
    UPROPERTY()
    float Level = -1;
};

USTRUCT()
struct FActiveGameplayEffect : public FFastArraySerializerItem {
    GENERATED_BODY()
    UPROPERTY(Transient)
    FActiveGameplayEffectHandle Handle;
    UPROPERTY()
    FGameplayEffectSpec Spec;
    UPROPERTY()
    FPredictionKey PredictionKey;
    UPROPERTY()
    TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
    UPROPERTY()
    float StartServerWorldTime = 0;
    UPROPERTY()
    float CachedStartServerWorldTime = 0;
    UPROPERTY()
    float StartWorldTime = 0;
    UPROPERTY()
    int32 StartGameStateTime = 0;
    UPROPERTY()
    bool IsInhibited = false;
    UPROPERTY()
    bool bIsInhibited = true;
};

USTRUCT()
struct FActiveGameplayEffectsContainer : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FActiveGameplayEffect> GameplayEffects;
    UPROPERTY()
    TArray<FActiveGameplayEffect> GameplayEffects_Internal;
    UPROPERTY()
    TArray<TObjectPtr<const UGameplayEffect>> ApplicationImmunityQueryEffects;
};

}
