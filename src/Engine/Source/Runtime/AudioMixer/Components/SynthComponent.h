#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/IAudioExtensionPlugin.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundAttenuation.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundModulationDestination.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundSourceBusSend.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundSubmixSend.h"

#include "SynthComponent.generated.h"

namespace ue5parse::runtime {

class UAudioComponent;
class USoundAttenuation;
class USoundClass;
class USoundConcurrency;
class USoundEffectSourcePresetChain;
class USoundSubmixBase;
class USynthSound;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSynthEnvelopeValue, float, EnvelopeValue);

UCLASS()
class USynthComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit USynthComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    uint8 bAutoDestroy : 1 = false;
    UPROPERTY()
    uint8 bStopWhenOwnerDestroyed : 1 = true;
    UPROPERTY()
    uint8 bAllowSpatialization : 1 = false;
    UPROPERTY()
    uint8 bOverrideAttenuation : 1 = false;
    UPROPERTY()
    uint32 bOutputToBusOnly_DEPRECATED : 1 = false;
    UPROPERTY()
    uint32 bEnableBusSends : 1 = true;
    UPROPERTY()
    uint32 bEnableBaseSubmix : 1 = true;
    UPROPERTY()
    uint32 bEnableSubmixSends : 1 = true;
    UPROPERTY()
    TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;
    UPROPERTY()
    FSoundAttenuationSettings AttenuationOverrides;
    UPROPERTY()
    TObjectPtr<USoundConcurrency> ConcurrencySettings_DEPRECATED = nullptr;
    UPROPERTY()
    TSet<TObjectPtr<USoundConcurrency>> ConcurrencySet;
    UPROPERTY()
    TObjectPtr<USoundClass> SoundClass = nullptr;
    UPROPERTY()
    TObjectPtr<USoundEffectSourcePresetChain> SourceEffectChain = nullptr;
    UPROPERTY()
    TObjectPtr<USoundSubmixBase> SoundSubmix = nullptr;
    UPROPERTY()
    TArray<FSoundSubmixSendInfo> SoundSubmixSends;
    UPROPERTY()
    TArray<FSoundSourceBusSendInfo> BusSends;
    UPROPERTY()
    TArray<FSoundSourceBusSendInfo> PreEffectBusSends;
    UPROPERTY()
    FSoundModulation Modulation;
    UPROPERTY()
    FSoundModulationDefaultRoutingSettings ModulationRouting;
    UPROPERTY()
    uint8 bIsUISound : 1 = false;
    UPROPERTY()
    uint8 bIsPreviewSound : 1 = false;
    UPROPERTY()
    int32 EnvelopeFollowerAttackTime = 0;
    UPROPERTY()
    int32 EnvelopeFollowerReleaseTime = 0;
    UPROPERTY()
    FOnSynthEnvelopeValue OnAudioEnvelopeValue;
    UPROPERTY()
    TObjectPtr<USynthSound> Synth = nullptr;
    UPROPERTY()
    TObjectPtr<UAudioComponent> AudioComponent = nullptr;
};

}
