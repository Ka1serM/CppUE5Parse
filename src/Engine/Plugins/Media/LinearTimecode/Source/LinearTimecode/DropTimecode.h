#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "DropTimecode.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FDropTimecode {
    GENERATED_BODY()

    UPROPERTY()
    FTimecode Timecode;
    UPROPERTY()
    int32 FrameRate = 0;
    UPROPERTY()
    bool bColorFraming = false;
    UPROPERTY()
    bool bRunningForward = false;
    UPROPERTY()
    bool bNewFrame = false;
};

}
