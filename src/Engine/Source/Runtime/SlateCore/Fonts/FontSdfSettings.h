#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "FontSdfSettings.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FFontSdfSettings {
    GENERATED_BODY()
    UPROPERTY()
    int32 BasePpem = 32;
};

}
