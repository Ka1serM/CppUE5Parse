#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "IntMargin.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FIntMargin {
    GENERATED_BODY()
    UPROPERTY()
    int32 Left = 0;
    UPROPERTY()
    int32 Top = 0;
    UPROPERTY()
    int32 Right = 0;
    UPROPERTY()
    int32 Bottom = 0;
};

}
