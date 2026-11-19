#pragma once

#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

#include "GameplayAbilityTriggerType.generated.h"

namespace ue5parse::runtime {

namespace EGameplayAbilityTriggerSource {

UENUM()
enum Type : int32 { GameplayEvent, OwnedTagAdded, OwnedTagPresent };

}

USTRUCT()
struct FAbilityTriggerData {
    GENERATED_BODY()
    UPROPERTY()
    FGameplayTag TriggerTag;
    UPROPERTY()
    TEnumAsByte<EGameplayAbilityTriggerSource::Type> TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
};

}
