#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "AudioBus.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EAudioBusChannels : uint8 {
    Mono,
    Stereo,
    Quad,
    FivePointOne,
    SevenPointOne,
    MaxChannelCount,
};

UCLASS()
class UAudioBus : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    EAudioBusChannels AudioBusChannels = EAudioBusChannels::Mono;
    UPROPERTY()
    bool bIsAutomatic = false;
};

}
