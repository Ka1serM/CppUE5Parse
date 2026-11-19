#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "DataRegistryId.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FDataRegistryType {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
};

}
