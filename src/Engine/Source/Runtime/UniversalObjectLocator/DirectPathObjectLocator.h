#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "DirectPathObjectLocator.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FDirectPathObjectLocator {
    GENERATED_BODY()

    UPROPERTY()
    FSoftObjectPath Path;
};

}
