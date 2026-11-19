#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "GameplayAbilitySpecHandle.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FGameplayAbilitySpecHandle {
    GENERATED_BODY()
    UPROPERTY()
    int32 Handle_DEPRECATED = INDEX_NONE;
};

}
