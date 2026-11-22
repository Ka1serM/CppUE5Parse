#pragma once

#include "Engine/Source/Runtime/AudioMixer/Components/SynthComponent.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "MediaSoundComponent.generated.h"

namespace ue5parse::runtime {

class UMediaPlayer;

UENUM()
enum class EMediaSoundChannels : int32 { Mono, Stereo, Surround };

UCLASS()
class UMediaSoundComponent : public USynthComponent {
    GENERATED_BODY()
public:
    explicit UMediaSoundComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    EMediaSoundChannels Channels = EMediaSoundChannels::Stereo;
    UPROPERTY()
    bool DynamicRateAdjustment = false;
    UPROPERTY()
    float RateAdjustmentFactor = 0.00000001f;
    UPROPERTY()
    FFloatRange RateAdjustmentRange{0.995f, 1.005f};
    UPROPERTY()
    TObjectPtr<UMediaPlayer> MediaPlayer = nullptr;
};

}
