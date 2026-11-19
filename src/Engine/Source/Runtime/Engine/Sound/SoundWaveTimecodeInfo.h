#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "SoundWaveTimecodeInfo.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSoundWaveTimecodeInfo {
    GENERATED_BODY()
    UPROPERTY()
    uint64 NumSamplesSinceMidnight = ~0ull;
    UPROPERTY()
    uint32 NumSamplesPerSecond = 0;
    UPROPERTY()
    FString Description;
    UPROPERTY()
    FString OriginatorTime;
    UPROPERTY()
    FString OriginatorDate;
    UPROPERTY()
    FString OriginatorDescription;
    UPROPERTY()
    FString OriginatorReference;
    UPROPERTY()
    FFrameRate TimecodeRate;
    UPROPERTY()
    bool bTimecodeIsDropFrame = false;
};

}
