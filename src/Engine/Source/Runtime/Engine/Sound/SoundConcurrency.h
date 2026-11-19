#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SoundConcurrency.generated.h"

namespace ue5parse::runtime {

namespace EMaxConcurrentResolutionRule {

UENUM()
enum Type : uint8 {
    PreventNew,
    StopOldest,
    StopFarthestThenPreventNew,
    StopFarthestThenOldest,
    StopLowestPriority,
    StopQuietest,
    StopLowestPriorityThenPreventNew,
};

}

UENUM()
enum class EConcurrencyVolumeScaleMode {
    Default,
    Distance,
    Priority,
};

USTRUCT()
struct FSoundConcurrencySettings {
    GENERATED_BODY()
    UPROPERTY()
    int32 MaxCount = 16;
    UPROPERTY()
    uint8 bLimitToOwner : 1 = 0;
    UPROPERTY()
    uint8 bVolumeScaleCanRelease : 1 = 0;
    UPROPERTY()
    uint8 bEnableMaxCountPlatformScaling : 1 = 0;
    UPROPERTY()
    FPerPlatformInt PlatformMaxCount;
    UPROPERTY()
    TEnumAsByte<EMaxConcurrentResolutionRule::Type> ResolutionRule = EMaxConcurrentResolutionRule::StopFarthestThenOldest;
    UPROPERTY()
    float RetriggerTime = 0.0f;
    UPROPERTY()
    float VolumeScale = 1.0f;
    UPROPERTY()
    EConcurrencyVolumeScaleMode VolumeScaleMode = EConcurrencyVolumeScaleMode::Default;
    UPROPERTY()
    float VolumeScaleAttackTime = 0.01f;
    UPROPERTY()
    float VolumeScaleReleaseTime = 0.5f;
    UPROPERTY()
    float VoiceStealReleaseTime = 0.0f;
};

UCLASS()
class USoundConcurrency : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSoundConcurrencySettings Concurrency;
};

}
