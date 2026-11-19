#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/Attenuation.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundSubmixSend.h"

#include "SoundAttenuation.generated.h"

namespace ue5parse::runtime {

class UAudioLinkSettingsAbstract;
class UOcclusionPluginSourceSettingsBase;
class UReverbPluginSourceSettingsBase;
class USoundSubmixBase;
class USourceDataOverridePluginSourceSettingsBase;
class USpatializationPluginSourceSettingsBase;

UENUM()
enum ESoundDistanceCalc : int32 {
    SOUNDDISTANCE_Normal,
    SOUNDDISTANCE_InfiniteXYPlane,
    SOUNDDISTANCE_InfiniteXZPlane,
    SOUNDDISTANCE_InfiniteYZPlane,
};

UENUM()
enum ESoundSpatializationAlgorithm : int32 {
    SPATIALIZATION_Default,
    SPATIALIZATION_HRTF,
};

UENUM()
enum class EAirAbsorptionMethod : uint8 {
    Linear,
    CustomCurve,
};

UENUM()
enum class EReverbSendMethod : uint8 {
    Linear,
    CustomCurve,
    Manual,
};

UENUM()
enum class EPriorityAttenuationMethod : uint8 {
    Linear,
    CustomCurve,
    Manual,
};

UENUM()
enum class ESubmixSendMethod : uint8 {
    Linear,
    CustomCurve,
    Manual,
};

UENUM()
enum class ENonSpatializedRadiusSpeakerMapMode : uint8 {
    OmniDirectional,
    Direct2D,
    Surround2D,
};

USTRUCT()
struct FSoundAttenuationPluginSettings {
    GENERATED_BODY()
    UPROPERTY()
    TArray<TObjectPtr<USpatializationPluginSourceSettingsBase>> SpatializationPluginSettingsArray;
    UPROPERTY()
    TArray<TObjectPtr<UOcclusionPluginSourceSettingsBase>> OcclusionPluginSettingsArray;
    UPROPERTY()
    TArray<TObjectPtr<UReverbPluginSourceSettingsBase>> ReverbPluginSettingsArray;
    UPROPERTY()
    TArray<TObjectPtr<USourceDataOverridePluginSourceSettingsBase>> SourceDataOverridePluginSettingsArray;
};

USTRUCT()
struct FAttenuationSubmixSendSettings : public FSoundSubmixSendInfoBase {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<USoundSubmixBase> Submix = nullptr;
    UPROPERTY()
    ESubmixSendMethod SubmixSendMethod = ESubmixSendMethod::Linear;
    UPROPERTY()
    float SubmixSendLevelMin = 0.0f;
    UPROPERTY()
    float SubmixSendLevelMax = 1.0f;
    UPROPERTY()
    float SubmixSendDistanceMin = 400.0f;
    UPROPERTY()
    float SubmixSendDistanceMax = 6000.0f;
    UPROPERTY()
    float ManualSubmixSendLevel = 0.2f;
    UPROPERTY()
    FRuntimeFloatCurve CustomSubmixSendCurve;
};

USTRUCT()
struct FSoundAttenuationSettings : public FBaseAttenuationSettings {
    GENERATED_BODY()
    UPROPERTY()
    uint8 bAttenuate : 1 = 1;
    UPROPERTY()
    uint8 bSpatialize : 1 = 1;
    UPROPERTY()
    uint8 bAttenuateWithLPF : 1 = 0;
    UPROPERTY()
    uint8 bEnableListenerFocus : 1 = 0;
    UPROPERTY()
    uint8 bEnableFocusInterpolation : 1 = 0;
    UPROPERTY()
    uint8 bEnableOcclusion : 1 = 0;
    UPROPERTY()
    uint8 bUseComplexCollisionForOcclusion : 1 = 0;
    UPROPERTY()
    uint8 bEnableReverbSend : 1 = 1;
    UPROPERTY()
    uint8 bEnablePriorityAttenuation : 1 = 0;
    UPROPERTY()
    uint8 bApplyNormalizationToStereoSounds : 1 = 0;
    UPROPERTY()
    uint8 bEnableLogFrequencyScaling : 1 = 0;
    UPROPERTY()
    uint8 bEnableSubmixSends : 1 = 0;
    UPROPERTY()
    uint8 bEnableSourceDataOverride : 1 = 0;
    UPROPERTY()
    uint8 bEnableSendToAudioLink : 1 = 1;
    UPROPERTY()
    TEnumAsByte<ESoundSpatializationAlgorithm> SpatializationAlgorithm = SPATIALIZATION_Default;
    UPROPERTY()
    float BinauralRadius = 0.0f;
    UPROPERTY()
    EAirAbsorptionMethod AbsorptionMethod = EAirAbsorptionMethod::Linear;
    UPROPERTY()
    TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;
    UPROPERTY()
    EReverbSendMethod ReverbSendMethod = EReverbSendMethod::Linear;
    UPROPERTY()
    EPriorityAttenuationMethod PriorityAttenuationMethod = EPriorityAttenuationMethod::Linear;
    UPROPERTY()
    ENonSpatializedRadiusSpeakerMapMode NonSpatializedRadiusMode = ENonSpatializedRadiusSpeakerMapMode::OmniDirectional;
    UPROPERTY()
    float NonSpatializedRadiusStart = 0.0f;
    UPROPERTY()
    float NonSpatializedRadiusEnd = 0.0f;
    UPROPERTY()
    TEnumAsByte<ESoundDistanceCalc> DistanceType_DEPRECATED = SOUNDDISTANCE_Normal;
    UPROPERTY()
    float OmniRadius_DEPRECATED = 0.0f;
    UPROPERTY()
    float StereoSpread = 200.0f;
    UPROPERTY()
    float RadiusMin_DEPRECATED = 400.0f;
    UPROPERTY()
    float RadiusMax_DEPRECATED = 4000.0f;
    UPROPERTY()
    float LPFRadiusMin = 3000.0f;
    UPROPERTY()
    float LPFRadiusMax = 6000.0f;
    UPROPERTY()
    FRuntimeFloatCurve CustomLowpassAirAbsorptionCurve;
    UPROPERTY()
    FRuntimeFloatCurve CustomHighpassAirAbsorptionCurve;
    UPROPERTY()
    float LPFFrequencyAtMin = 20000.0f;
    UPROPERTY()
    float LPFFrequencyAtMax = 20000.0f;
    UPROPERTY()
    float HPFFrequencyAtMin = 0.0f;
    UPROPERTY()
    float HPFFrequencyAtMax = 0.0f;
    UPROPERTY()
    float FocusAzimuth = 30.0f;
    UPROPERTY()
    float NonFocusAzimuth = 60.0f;
    UPROPERTY()
    float FocusDistanceScale = 1.0f;
    UPROPERTY()
    float NonFocusDistanceScale = 1.0f;
    UPROPERTY()
    float FocusPriorityScale = 1.0f;
    UPROPERTY()
    float NonFocusPriorityScale = 1.0f;
    UPROPERTY()
    float FocusVolumeAttenuation = 1.0f;
    UPROPERTY()
    float NonFocusVolumeAttenuation = 1.0f;
    UPROPERTY()
    float FocusAttackInterpSpeed = 1.0f;
    UPROPERTY()
    float FocusReleaseInterpSpeed = 1.0f;
    UPROPERTY()
    float OcclusionLowPassFilterFrequency = 20000.0f;
    UPROPERTY()
    float OcclusionVolumeAttenuation = 1.0f;
    UPROPERTY()
    float OcclusionInterpolationTime = 0.1f;
    UPROPERTY()
    float ReverbWetLevelMin = 0.3f;
    UPROPERTY()
    float ReverbWetLevelMax = 0.95f;
    UPROPERTY()
    float ReverbDistanceMin = 0.0f;
    UPROPERTY()
    float ReverbDistanceMax = 0.0f;
    UPROPERTY()
    float ManualReverbSendLevel = 0.2f;
    UPROPERTY()
    FRuntimeFloatCurve CustomReverbSendCurve;
    UPROPERTY()
    TArray<FAttenuationSubmixSendSettings> SubmixSendSettings;
    UPROPERTY()
    float PriorityAttenuationMin = 1.0f;
    UPROPERTY()
    float PriorityAttenuationMax = 1.0f;
    UPROPERTY()
    float PriorityAttenuationDistanceMin = 0.0f;
    UPROPERTY()
    float PriorityAttenuationDistanceMax = 0.0f;
    UPROPERTY()
    float ManualPriorityAttenuation = 1.0f;
    UPROPERTY()
    FRuntimeFloatCurve CustomPriorityAttenuationCurve;
    UPROPERTY()
    FSoundAttenuationPluginSettings PluginSettings;
    UPROPERTY()
    TObjectPtr<UAudioLinkSettingsAbstract> AudioLinkSettingsOverride = nullptr;
    UPROPERTY()
    TObjectPtr<USpatializationPluginSourceSettingsBase> SpatializationPluginSettings_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UOcclusionPluginSourceSettingsBase> OcclusionPluginSettings_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UReverbPluginSourceSettingsBase> ReverbPluginSettings_DEPRECATED = nullptr;
};

UCLASS()
class USoundAttenuation : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSoundAttenuationSettings Attenuation;
};

}
