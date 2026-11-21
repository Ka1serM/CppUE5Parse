#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "SubObjectLocator.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSubObjectLocator {
    GENERATED_BODY()

    UPROPERTY()
    FString PathWithinContext;
};

}
