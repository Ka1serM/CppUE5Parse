#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundWave.h"

#include "SoundSourceBus.generated.h"

namespace ue5parse::runtime {

class UAudioBus;

UENUM()
enum class ESourceBusChannels : uint8 {
    Mono,
    Stereo,
};

UCLASS()
class USoundSourceBus : public USoundWave {
    GENERATED_BODY()
public:
    UPROPERTY()
    ESourceBusChannels SourceBusChannels = ESourceBusChannels::Mono;
    UPROPERTY()
    float SourceBusDuration = 0.0f;
    UPROPERTY()
    TObjectPtr<UAudioBus> AudioBus = nullptr;
    UPROPERTY()
    uint32 bAutoDeactivateWhenSilent : 1 = 1;

    void SerializeCuePoints(FArchive& Ar, bool bIsLoadingFromCookedArchive) override;
    void SerializeCookedPlatformData(FArchive& Ar) override;
};

}
