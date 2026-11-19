#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/IAudioExtensionPlugin.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundModulationDestination.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWaveLoadingBehavior.h"

#include "SoundClass.generated.h"

namespace ue5parse::runtime {

class USoundMix;
class USoundSubmix;

namespace EAudioOutputTarget {

UENUM()
enum Type : uint8 { Speaker, Controller, ControllerFallbackToSpeaker };

}

USTRUCT()
struct FSoundClassProperties {
    GENERATED_BODY()
    UPROPERTY()
    float Volume = 1.0f;
    UPROPERTY()
    float Pitch = 1.0f;
    UPROPERTY()
    float LowPassFilterFrequency = 20000.0f;
    UPROPERTY()
    float AttenuationDistanceScale = 1.0f;
    UPROPERTY()
    float StereoBleed = 0.25f;
    UPROPERTY()
    float LFEBleed = 0.5f;
    UPROPERTY()
    float VoiceCenterChannelVolume = 0.0f;
    UPROPERTY()
    float RadioFilterVolume = 0.0f;
    UPROPERTY()
    float RadioFilterVolumeThreshold = 0.0f;
    UPROPERTY()
    uint8 bApplyEffects : 1 = 0;
    UPROPERTY()
    uint8 bAlwaysPlay : 1 = 0;
    UPROPERTY()
    uint8 bIsUISound : 1 = 0;
    UPROPERTY()
    uint8 bIsMusic : 1 = 0;
    UPROPERTY()
    uint8 bReverb : 1 = 1;
    UPROPERTY()
    float Default2DReverbSendAmount = 0.0f;
    UPROPERTY()
    FSoundModulationDefaultSettings ModulationSettings;
    UPROPERTY()
    uint8 bCenterChannelOnly : 1 = 0;
    UPROPERTY()
    uint8 bApplyAmbientVolumes : 1 = 0;
    UPROPERTY()
    TEnumAsByte<EAudioOutputTarget::Type> OutputTarget = EAudioOutputTarget::Speaker;
    UPROPERTY()
    ESoundWaveLoadingBehavior LoadingBehavior = ESoundWaveLoadingBehavior::Inherited;
    UPROPERTY()
    FPerPlatformFloat SizeOfFirstAudioChunkInSeconds;
    UPROPERTY()
    TObjectPtr<USoundSubmix> DefaultSubmix = nullptr;
};

USTRUCT()
struct FPassiveSoundMixModifier {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<USoundMix> SoundMix = nullptr;
    UPROPERTY()
    float MinVolumeThreshold = 0.0f;
    UPROPERTY()
    float MaxVolumeThreshold = 10.0f;
};

USTRUCT()
struct FSoundClassEditorData {
    GENERATED_BODY()
    int32 NodePosX = 0;
    int32 NodePosY = 0;

    friend FArchive& operator<<(FArchive& Ar, FSoundClassEditorData& EditorData);
};

UCLASS()
class USoundClass : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSoundClassProperties Properties;
    UPROPERTY()
    TArray<TObjectPtr<USoundClass>> ChildClasses;
    UPROPERTY()
    TArray<FPassiveSoundMixModifier> PassiveSoundMixModifiers;
    UPROPERTY()
    FSoundModulation Modulation;
    UPROPERTY()
    TObjectPtr<USoundClass> ParentClass = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
    UPROPERTY()
    TArray<FName> ChildClassNames_DEPRECATED;

    void Serialize(FArchive& Ar) override;
};

}
