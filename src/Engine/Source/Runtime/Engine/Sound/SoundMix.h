#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SoundMix.generated.h"

namespace ue5parse::runtime {

class USoundClass;

USTRUCT()
struct FAudioEQEffect {
    GENERATED_BODY()
    UPROPERTY()
    float FrequencyCenter0 = 600.0f;
    UPROPERTY()
    float Gain0 = 1.0f;
    UPROPERTY()
    float Bandwidth0 = 1.0f;
    UPROPERTY()
    float FrequencyCenter1 = 1000.0f;
    UPROPERTY()
    float Gain1 = 1.0f;
    UPROPERTY()
    float Bandwidth1 = 1.0f;
    UPROPERTY()
    float FrequencyCenter2 = 2000.0f;
    UPROPERTY()
    float Gain2 = 1.0f;
    UPROPERTY()
    float Bandwidth2 = 1.0f;
    UPROPERTY()
    float FrequencyCenter3 = 10000.0f;
    UPROPERTY()
    float Gain3 = 1.0f;
    UPROPERTY()
    float Bandwidth3 = 1.0f;
    UPROPERTY()
    float HFFrequency = 0.0f;
    UPROPERTY()
    float HFGain = 0.0f;
    UPROPERTY()
    float MFCutoffFrequency = 0.0f;
    UPROPERTY()
    float MFBandwidth = 0.0f;
    UPROPERTY()
    float MFGain = 0.0f;
    UPROPERTY()
    float LFFrequency = 0.0f;
    UPROPERTY()
    float LFGain = 0.0f;
};

USTRUCT()
struct FSoundClassAdjuster {
    GENERATED_BODY()
    UPROPERTY()
    FName SoundClass_DEPRECATED;
    UPROPERTY()
    TObjectPtr<USoundClass> SoundClassObject = nullptr;
    UPROPERTY()
    float VolumeAdjuster = 1.0f;
    UPROPERTY()
    float PitchAdjuster = 1.0f;
    UPROPERTY()
    float LowPassFilterFrequency = 20000.0f;
    UPROPERTY()
    uint32 bApplyToChildren : 1 = 0;
    UPROPERTY()
    float VoiceCenterChannelVolumeAdjuster = 1.0f;
};

UCLASS()
class USoundMix : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint32 bApplyEQ : 1 = 0;
    UPROPERTY()
    float EQPriority = 0.0f;
    UPROPERTY()
    FAudioEQEffect EQSettings;
    UPROPERTY()
    TArray<FSoundClassAdjuster> SoundClassEffects;
    UPROPERTY()
    float InitialDelay = 0.0f;
    UPROPERTY()
    float FadeInTime = 0.2f;
    UPROPERTY()
    float Duration = -1.0f;
    UPROPERTY()
    float FadeOutTime = 0.2f;
    UPROPERTY()
    uint32 bChanged : 1 = 0;
};

}
