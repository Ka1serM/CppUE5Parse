#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "SoundTimecodeOffset.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSoundTimecodeOffset {
    GENERATED_BODY()
    UPROPERTY()
    double NumOfSecondsSinceMidnight = 0.0;
};

}
