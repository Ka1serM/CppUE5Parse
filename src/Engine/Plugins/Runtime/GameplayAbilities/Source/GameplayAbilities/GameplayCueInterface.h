#pragma once

#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayEffectTypes.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/GameplayPrediction.h"

#include "GameplayCueInterface.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FActiveGameplayCue : public FFastArraySerializerItem {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayTag GameplayCueTag;
    UPROPERTY()
    FPredictionKey PredictionKey;
    UPROPERTY()
    FGameplayCueParameters Parameters;
    UPROPERTY()
    bool bPredictivelyRemoved = false;
};

USTRUCT()
struct FActiveGameplayCueContainer : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FActiveGameplayCue> GameplayCues;
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> Owner = nullptr;
};

}
