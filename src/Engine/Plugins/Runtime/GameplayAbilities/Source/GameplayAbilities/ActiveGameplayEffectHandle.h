#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"

#include "ActiveGameplayEffectHandle.generated.h"

namespace ue5parse::runtime {

class UAbilitySystemComponent;

USTRUCT()
struct FActiveGameplayEffectHandle {
    GENERATED_BODY()
    UPROPERTY()
    TWeakObjectPtr<UAbilitySystemComponent> WeakOwningASC;
    UPROPERTY()
    bool bPassedFiltersAndWasExecuted = false;
    UPROPERTY()
    int32 Handle_DEPRECATED = INDEX_NONE;
};

}
