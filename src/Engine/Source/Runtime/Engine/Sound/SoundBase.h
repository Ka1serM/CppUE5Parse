#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/IAudioExtensionPlugin.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundConcurrency.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundModulationDestination.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundSourceBusSend.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundSubmixSend.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundTimecodeOffset.h"

#include "SoundBase.generated.h"

namespace ue5parse::runtime {

class UAudioPropertiesBindings;
class UAudioPropertiesSheetAssetBase;
class USoundAttenuation;
class USoundClass;
class USoundEffectSourcePresetChain;
class USoundSubmixBase;

UENUM()
enum class EVirtualizationMode : uint8 {
    Disabled,
    PlayWhenSilent,
    Restart,
    SeekRestart,
};

UENUM()
enum class EUseSubmixOnPreviewMode : uint8 {
    UseEditorPreference,
    UseSubmixesOnPreview,
    PlayPreviewsWithoutSubmixes,
};

USTRUCT()
struct FSoundBaseEditorData {
    GENERATED_BODY()
    UPROPERTY()
    EUseSubmixOnPreviewMode OverrideUseSubmixesOnPreview = EUseSubmixOnPreviewMode::UseEditorPreference;
};

UCLASS(Abstract)
class USoundBase : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName SoundClass_DEPRECATED;
    UPROPERTY()
    TObjectPtr<USoundClass> SoundClassObject = nullptr;
    UPROPERTY()
    uint8 bDebug : 1 = 0;
    UPROPERTY()
    uint8 bOverrideConcurrency : 1 = 0;
    UPROPERTY()
    uint8 bEnableBusSends : 1 = 1;
    UPROPERTY()
    uint8 bEnableBaseSubmix : 1 = 1;
    UPROPERTY()
    uint8 bEnableSubmixSends : 1 = 1;
    UPROPERTY()
    uint8 bHasDelayNode : 1 = 0;
    UPROPERTY()
    uint8 bHasConcatenatorNode : 1 = 0;
    UPROPERTY()
    uint8 bHasVirtualizeWhenSilent_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bBypassVolumeScaleForPriority : 1 = 0;
    UPROPERTY()
    uint8 bIgnoreFocus_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bOutputToBusOnly_DEPRECATED : 1 = 0;
    UPROPERTY()
    EVirtualizationMode VirtualizationMode = EVirtualizationMode::Restart;
    UPROPERTY()
    TSet<TObjectPtr<USoundConcurrency>> ConcurrencySet;
    UPROPERTY()
    FSoundConcurrencySettings ConcurrencyOverrides;
    UPROPERTY()
    TEnumAsByte<EMaxConcurrentResolutionRule::Type> MaxConcurrentResolutionRule_DEPRECATED = EMaxConcurrentResolutionRule::StopFarthestThenOldest;
    UPROPERTY()
    int32 MaxConcurrentPlayCount_DEPRECATED = 16;
    UPROPERTY()
    float Duration = 0.0f;
    UPROPERTY()
    float MaxDistance = 0.0f;
    UPROPERTY()
    float TotalSamples = 0.0f;
    UPROPERTY()
    float Priority = 1.0f;
    UPROPERTY()
    TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;
    UPROPERTY()
    FSoundModulation Modulation;
    UPROPERTY()
    FSoundModulationDefaultRoutingSettings ModulationSettings;
    UPROPERTY()
    TObjectPtr<USoundSubmixBase> SoundSubmixObject = nullptr;
    UPROPERTY()
    TArray<FSoundSubmixSendInfo> SoundSubmixSends;
    UPROPERTY()
    TObjectPtr<USoundEffectSourcePresetChain> SourceEffectChain = nullptr;
    UPROPERTY()
    float DefaultMasterReverbSendAmount = 0.0f;
    UPROPERTY()
    TArray<FSoundSourceBusSendInfo> BusSends;
    UPROPERTY()
    TArray<FSoundSourceBusSendInfo> PreEffectBusSends;
    UPROPERTY()
    TArray<FSoundSourceBusSendInfo> PostAttenuationBusSends;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
    UPROPERTY()
    FSoundTimecodeOffset TimecodeOffset;
    UPROPERTY()
    TObjectPtr<UAudioPropertiesSheetAssetBase> AudioPropertiesSheet = nullptr;
    UPROPERTY()
    TObjectPtr<UAudioPropertiesBindings> AudioPropertiesBindings = nullptr;
    UPROPERTY()
    TArray<FName> LocalAudioProperties;
    UPROPERTY()
    FSoundBaseEditorData EditorData;
    UPROPERTY()
    TObjectPtr<USoundConcurrency> SoundConcurrencySettings_DEPRECATED = nullptr;
};

}
