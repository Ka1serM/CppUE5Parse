#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "Scene.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;
class UObject;
class UTexture;
class UTexture2D;
class UTextureCube;

UENUM()
enum EDepthOfFieldMethod : int32 { DOFM_BokehDOF, DOFM_Gaussian, DOFM_CircleDOF, DOFM_MAX };

UENUM()
enum EAntiAliasingMethod : int32 { AAM_None, AAM_FXAA, AAM_TemporalAA, AAM_MSAA, AAM_TSR, AAM_SMAA, AAM_MAX };

UENUM()
enum EAutoExposureMethod : int32 { AEM_Histogram, AEM_Basic, AEM_Manual, AEM_MAX };

UENUM()
enum EBloomMethod : int32 { BM_SOG, BM_FFT, BM_MAX };

UENUM()
enum class ELightUnits : uint8 { Unitless, Candelas, Lumens, EV, Nits };

UENUM()
enum class EReflectionsType : uint8 { ScreenSpace, RayTracing };

UENUM()
enum class ETranslucencyType : uint8 { Raster = 0, RayTraced_Deprecated = 1, RayTracing = 1, RayTraced = 2 };

UENUM()
enum class EReflectedAndRefractedRayTracedShadows : uint8 { Disabled, Hard_shadows, Area_shadows };

UENUM()
enum class ERayTracingGlobalIlluminationType : uint8 { Disabled, BruteForce, FinalGather };

UENUM()
enum ETemperatureMethod : int32 { TEMP_WhiteBalance, TEMP_ColorTemperature, TEMP_MAX };

UENUM()
enum class ELumenRayLightingModeOverride : uint8 { Default, SurfaceCache, HitLightingForReflections, HitLighting };

UENUM()
enum class ELocalExposureMethod : uint8 { Bilateral, Fusion };

UENUM()
enum class ETonemappingMethod : uint8 { Default = 2, Filmic = 0, StandardACES = 1, MAX = 3 };

UENUM()
enum class EVignetteType : uint8 { CosineFourthLaw = 0, Texture = 1, Oval = 2 };

USTRUCT()
struct FWeightedBlendable {
    GENERATED_BODY()
    UPROPERTY()
    float Weight = -1.0f;
    UPROPERTY()
    TObjectPtr<UObject> Object = nullptr;
};

USTRUCT()
struct FWeightedBlendables {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FWeightedBlendable> Array;
};

USTRUCT()
struct FMatteBoxFlag {
    GENERATED_BODY()
    UPROPERTY()
    float Pitch = 0.0f;
    UPROPERTY()
    float Roll = 0.0f;
    UPROPERTY()
    float Length = 0.0f;
};

USTRUCT()
struct FPostProcessSettings {
    GENERATED_BODY()
    UPROPERTY()
    uint8 bOverride_FilmWhitePoint : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmSaturation : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmChannelMixerRed : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmChannelMixerGreen : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmChannelMixerBlue : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmContrast : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmDynamicRange : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmHealAmount : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmToeAmount : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmShadowTint : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmShadowTintBlend : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmShadowTintAmount : 1 = false;
    UPROPERTY()
    uint8 bOverride_SceneColorTint : 1 = false;
    UPROPERTY()
    uint8 bOverride_SceneFringeIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_SceneFringeSaturation : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientCubemapTint : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientCubemapIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomThreshold : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom1Tint : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom1Size : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom2Size : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom2Tint : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom3Tint : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom3Size : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom4Tint : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom4Size : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom5Tint : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom5Size : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomDirtMaskIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomDirtMaskTint : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomDirtMask : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureLowPercent : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureHighPercent : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureMinBrightness : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureMaxBrightness : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureSpeedUp : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureSpeedDown : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureBias : 1 = false;
    UPROPERTY()
    uint8 bOverride_HistogramLogMin : 1 = false;
    UPROPERTY()
    uint8 bOverride_HistogramLogMax : 1 = false;
    UPROPERTY()
    uint8 bOverride_LensFlareIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_LensFlareTint : 1 = false;
    UPROPERTY()
    uint8 bOverride_LensFlareTints : 1 = false;
    UPROPERTY()
    uint8 bOverride_LensFlareBokehSize : 1 = false;
    UPROPERTY()
    uint8 bOverride_LensFlareBokehShape : 1 = false;
    UPROPERTY()
    uint8 bOverride_LensFlareThreshold : 1 = false;
    UPROPERTY()
    uint8 bOverride_VignetteIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_VignetteColor : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionStaticFraction : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionRadius : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionFadeDistance : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionFadeRadius : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionRadiusInWS : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionPower : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionBias : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionQuality : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionMipBlend : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionMipScale : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionMipThreshold : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVWarpIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVTransmissionIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_IndirectLightingColor : 1 = false;
    UPROPERTY()
    uint8 bOverride_IndirectLightingIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGradingIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGradingLUT : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldFocalDistance : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldFocalRegion : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldNearTransitionRegion : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldFarTransitionRegion : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldScale : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldMaxBokehSize : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldNearBlurSize : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldFarBlurSize : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldBokehShape : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldOcclusion : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldColorThreshold : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldSizeThreshold : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldSkyFocusDistance : 1 = false;
    UPROPERTY()
    uint8 bOverride_MotionBlurAmount : 1 = false;
    UPROPERTY()
    uint8 bOverride_MotionBlurMax : 1 = false;
    UPROPERTY()
    uint8 bOverride_MotionBlurPerObjectSize : 1 = false;
    UPROPERTY()
    uint8 bOverride_AntiAliasingMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_ScreenSpaceReflectionIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_ScreenSpaceReflectionQuality : 1 = false;
    UPROPERTY()
    uint8 bOverride_ScreenSpaceReflectionMaxRoughness : 1 = false;
    UPROPERTY()
    uint8 bOverride_ScreenSpaceReflectionRoughnessScale : 1 = false;
    UPROPERTY()
    FLinearColor FilmWhitePoint = FLinearColor(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FLinearColor FilmShadowTint = FLinearColor(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    float FilmShadowTintBlend = 0;
    UPROPERTY()
    float FilmShadowTintAmount = 0;
    UPROPERTY()
    float FilmSaturation = 0;
    UPROPERTY()
    FLinearColor FilmChannelMixerRed = FLinearColor(1.0f, 0.0f, 0.0f);
    UPROPERTY()
    FLinearColor FilmChannelMixerGreen = FLinearColor(0.0f, 1.0f, 0.0f);
    UPROPERTY()
    FLinearColor FilmChannelMixerBlue = FLinearColor(0.0f, 0.0f, 1.0f);
    UPROPERTY()
    float FilmContrast = 0;
    UPROPERTY()
    float FilmToeAmount = 0;
    UPROPERTY()
    float FilmHealAmount = 0;
    UPROPERTY()
    float FilmDynamicRange = 0;
    UPROPERTY()
    FLinearColor SceneColorTint = FLinearColor(1, 1, 1);
    UPROPERTY()
    float SceneFringeIntensity = 0.0f;
    UPROPERTY()
    float SceneFringeSaturation = 0;
    UPROPERTY()
    float BloomIntensity = 0.675f;
    UPROPERTY()
    float BloomThreshold = -1.0f;
    UPROPERTY()
    float Bloom1Size = 0.3f;
    UPROPERTY()
    float Bloom2Size = 1.0f;
    UPROPERTY()
    float Bloom3Size = 2.0f;
    UPROPERTY()
    float Bloom4Size = 10.0f;
    UPROPERTY()
    float Bloom5Size = 30.0f;
    UPROPERTY()
    FLinearColor Bloom1Tint = FLinearColor(0.3465f, 0.3465f, 0.3465f);
    UPROPERTY()
    FLinearColor Bloom2Tint = FLinearColor(0.138f, 0.138f, 0.138f);
    UPROPERTY()
    FLinearColor Bloom3Tint = FLinearColor(0.1176f, 0.1176f, 0.1176f);
    UPROPERTY()
    FLinearColor Bloom4Tint = FLinearColor(0.066f, 0.066f, 0.066f);
    UPROPERTY()
    FLinearColor Bloom5Tint = FLinearColor(0.066f, 0.066f, 0.066f);
    UPROPERTY()
    float BloomDirtMaskIntensity = 0.0f;
    UPROPERTY()
    FLinearColor BloomDirtMaskTint = FLinearColor(0.5f, 0.5f, 0.5f);
    UPROPERTY()
    TObjectPtr<UTexture> BloomDirtMask = nullptr;
    UPROPERTY()
    float LPVWarpIntensity = 0;
    UPROPERTY()
    float LPVTransmissionIntensity = 0;
    UPROPERTY()
    FLinearColor AmbientCubemapTint = FLinearColor(1, 1, 1);
    UPROPERTY()
    float AmbientCubemapIntensity = 1.0f;
    UPROPERTY()
    TObjectPtr<UTextureCube> AmbientCubemap = nullptr;
    UPROPERTY()
    float AutoExposureLowPercent = 10.0f;
    UPROPERTY()
    float AutoExposureHighPercent = 90.0f;
    UPROPERTY()
    float AutoExposureMinBrightness = 0.03f;
    UPROPERTY()
    float AutoExposureMaxBrightness = 8.0f;
    UPROPERTY()
    float AutoExposureSpeedUp = 3.0f;
    UPROPERTY()
    float AutoExposureSpeedDown = 1.0f;
    UPROPERTY()
    float AutoExposureBias = 1.0f;
    UPROPERTY()
    float HistogramLogMin = -8.0f;
    UPROPERTY()
    float HistogramLogMax = 4.0f;
    UPROPERTY()
    float LensFlareIntensity = 1.0f;
    UPROPERTY()
    FLinearColor LensFlareTint = FLinearColor(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    float LensFlareBokehSize = 3.0f;
    UPROPERTY()
    float LensFlareThreshold = 8.0f;
    UPROPERTY()
    TObjectPtr<UTexture> LensFlareBokehShape = nullptr;
    UPROPERTY()
    FLinearColor LensFlareTints[8] = {FLinearColor(1.0f, 0.8f, 0.4f, 0.6f),  FLinearColor(1.0f, 1.0f, 0.6f, 0.53f), FLinearColor(0.8f, 0.8f, 1.0f, 0.46f),
                                      FLinearColor(0.5f, 1.0f, 0.4f, 0.39f), FLinearColor(0.5f, 0.8f, 1.0f, 0.31f), FLinearColor(0.9f, 1.0f, 0.8f, 0.27f),
                                      FLinearColor(1.0f, 0.8f, 0.4f, 0.22f), FLinearColor(0.9f, 0.7f, 0.7f, 0.15f)};
    UPROPERTY()
    float VignetteIntensity = 0.4f;
    UPROPERTY()
    FLinearColor VignetteColor = FLinearColor::Black;
    UPROPERTY()
    float AmbientOcclusionIntensity = 0.5f;
    UPROPERTY()
    float AmbientOcclusionStaticFraction = 1.0f;
    UPROPERTY()
    float AmbientOcclusionRadius = 200.0f;
    UPROPERTY()
    uint8 AmbientOcclusionRadiusInWS : 1 = false;
    UPROPERTY()
    float AmbientOcclusionFadeDistance = 8000.0f;
    UPROPERTY()
    float AmbientOcclusionFadeRadius = 5000.0f;
    UPROPERTY()
    float AmbientOcclusionPower = 2.0f;
    UPROPERTY()
    float AmbientOcclusionBias = 3.0f;
    UPROPERTY()
    float AmbientOcclusionQuality = 50.0f;
    UPROPERTY()
    float AmbientOcclusionMipBlend = 0.6f;
    UPROPERTY()
    float AmbientOcclusionMipScale = 1.7f;
    UPROPERTY()
    float AmbientOcclusionMipThreshold = 0.01f;
    UPROPERTY()
    FLinearColor IndirectLightingColor = FLinearColor(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    float IndirectLightingIntensity = 1.0f;
    UPROPERTY()
    float ColorGradingIntensity = 1.0f;
    UPROPERTY()
    TObjectPtr<UTexture> ColorGradingLUT = nullptr;
    UPROPERTY()
    float DepthOfFieldFocalDistance = 0;
    UPROPERTY()
    float DepthOfFieldFocalRegion = 0.0f;
    UPROPERTY()
    float DepthOfFieldNearTransitionRegion = 300.0f;
    UPROPERTY()
    float DepthOfFieldFarTransitionRegion = 500.0f;
    UPROPERTY()
    float DepthOfFieldScale = 0.0f;
    UPROPERTY()
    float DepthOfFieldMaxBokehSize = 0;
    UPROPERTY()
    float DepthOfFieldNearBlurSize = 15.0f;
    UPROPERTY()
    float DepthOfFieldFarBlurSize = 15.0f;
    UPROPERTY()
    TObjectPtr<UTexture> DepthOfFieldBokehShape = nullptr;
    UPROPERTY()
    float DepthOfFieldOcclusion = 0.4f;
    UPROPERTY()
    float DepthOfFieldColorThreshold = 0;
    UPROPERTY()
    float DepthOfFieldSizeThreshold = 0;
    UPROPERTY()
    float DepthOfFieldSkyFocusDistance = 0.0f;
    UPROPERTY()
    float MotionBlurAmount = 0.5f;
    UPROPERTY()
    float MotionBlurMax = 5.0f;
    UPROPERTY()
    float MotionBlurPerObjectSize = 0.0f;
    UPROPERTY()
    TEnumAsByte<EAntiAliasingMethod> AntiAliasingMethod = AAM_None;
    UPROPERTY()
    float ScreenSpaceReflectionIntensity = 100.0f;
    UPROPERTY()
    float ScreenSpaceReflectionQuality = 50.0f;
    UPROPERTY()
    float ScreenSpaceReflectionMaxRoughness = 0.6f;
    UPROPERTY()
    uint8 bOverride_WhiteTemp : 1 = false;
    UPROPERTY()
    uint8 bOverride_WhiteTint : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorSaturation : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorContrast : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGamma : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGain : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorOffset : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmSlope : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmToe : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmShoulder : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmBlackClip : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmWhiteClip : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom6Tint : 1 = false;
    UPROPERTY()
    uint8 bOverride_Bloom6Size : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomSizeScale : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionDistance_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldFstop : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldDepthBlurRadius : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldDepthBlurAmount : 1 = false;
    UPROPERTY()
    float WhiteTemp = 6500.0f;
    UPROPERTY()
    float WhiteTint = 0.0f;
    UPROPERTY()
    FVector4 ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorContrast = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGamma = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGain = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorOffset = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    UPROPERTY()
    float FilmSlope = 0.88f;
    UPROPERTY()
    float FilmToe = 0.55f;
    UPROPERTY()
    float FilmShoulder = 0.26f;
    UPROPERTY()
    float FilmBlackClip = 0.0f;
    UPROPERTY()
    float FilmWhiteClip = 0.04f;
    UPROPERTY()
    float BloomSizeScale = 4.0;
    UPROPERTY()
    float Bloom6Size = 64.0f;
    UPROPERTY()
    FLinearColor Bloom6Tint = FLinearColor(0.061f, 0.061f, 0.061f);
    UPROPERTY()
    float AmbientOcclusionDistance_DEPRECATED = 80.0f;
    UPROPERTY()
    float DepthOfFieldDepthBlurAmount = 1.0f;
    UPROPERTY()
    float DepthOfFieldDepthBlurRadius = 0.0f;
    UPROPERTY()
    float DepthOfFieldFstop = 4.0f;
    UPROPERTY()
    FWeightedBlendables WeightedBlendables;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> Blendables_DEPRECATED;
    UPROPERTY()
    uint8 bOverride_AutoExposureMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldVignetteSize : 1 = false;
    UPROPERTY()
    TEnumAsByte<EAutoExposureMethod> AutoExposureMethod = AEM_Histogram;
    UPROPERTY()
    float DepthOfFieldVignetteSize = 200.0f;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldSensorWidth : 1 = false;
    UPROPERTY()
    uint8 bOverride_MobileHQGaussian : 1 = false;
    UPROPERTY()
    uint8 bMobileHQGaussian : 1 = false;
    UPROPERTY()
    float DepthOfFieldSensorWidth = 24.576f;
    UPROPERTY()
    uint8 bOverride_ColorSaturationShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorContrastShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGammaShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGainShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorOffsetShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorSaturationMidtones : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorContrastMidtones : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGammaMidtones : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGainMidtones : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorOffsetMidtones : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorSaturationHighlights : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorContrastHighlights : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGammaHighlights : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorGainHighlights : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorOffsetHighlights : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorCorrectionShadowsMax : 1 = false;
    UPROPERTY()
    uint8 bOverride_ColorCorrectionHighlightsMin : 1 = false;
    UPROPERTY()
    FVector4 ColorSaturationShadows = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorContrastShadows = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGammaShadows = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGainShadows = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorOffsetShadows = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    UPROPERTY()
    float ColorCorrectionShadowsMax = 0.09f;
    UPROPERTY()
    FVector4 ColorSaturationMidtones = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorContrastMidtones = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGammaMidtones = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGainMidtones = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorOffsetMidtones = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    UPROPERTY()
    FVector4 ColorSaturationHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorContrastHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGammaHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorGainHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    FVector4 ColorOffsetHighlights = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    UPROPERTY()
    float ColorCorrectionHighlightsMin = 0.5f;
    UPROPERTY()
    uint8 bOverride_BloomMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionTexture : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionSize : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionCenterUV : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionBufferScale : 1 = false;
    UPROPERTY()
    TEnumAsByte<EBloomMethod> BloomMethod = BM_SOG;
    UPROPERTY()
    TObjectPtr<UTexture2D> BloomConvolutionTexture = nullptr;
    UPROPERTY()
    float BloomConvolutionSize = 1.0f;
    UPROPERTY()
    FVector2D BloomConvolutionCenterUV = FVector2D(0.5f, 0.5f);
    UPROPERTY()
    float BloomConvolutionBufferScale = 0.133f;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionPreFilter_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionPreFilterMin : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionPreFilterMax : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionPreFilterMult : 1 = false;
    UPROPERTY()
    FVector3f BloomConvolutionPreFilter_DEPRECATED = FVector3f(-1.0f, -1.0f, -1.0f);
    UPROPERTY()
    float BloomConvolutionPreFilterMin = 7.0f;
    UPROPERTY()
    float BloomConvolutionPreFilterMax = 15000.0f;
    UPROPERTY()
    float BloomConvolutionPreFilterMult = 15.0f;
    UPROPERTY()
    uint8 bOverride_BlueCorrection : 1 = false;
    UPROPERTY()
    uint8 bOverride_ExpandGamut : 1 = false;
    UPROPERTY()
    uint8 bOverride_ChromaticAberrationStartOffset : 1 = false;
    UPROPERTY()
    uint8 bOverride_CameraShutterSpeed : 1 = false;
    UPROPERTY()
    uint8 bOverride_CameraISO : 1 = false;
    UPROPERTY()
    float BlueCorrection = 0.6f;
    UPROPERTY()
    float ExpandGamut = 1.0f;
    UPROPERTY()
    float ChromaticAberrationStartOffset = 0;
    UPROPERTY()
    float CameraShutterSpeed = 60.0f;
    UPROPERTY()
    float CameraISO = 100.0f;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldMinFstop : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldBladeCount : 1 = false;
    UPROPERTY()
    float DepthOfFieldMinFstop = 1.2f;
    UPROPERTY()
    int32 DepthOfFieldBladeCount = 5;
    UPROPERTY()
    uint8 bOverride_AutoExposureBiasCurve : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingAOSamplesPerPixel : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingReflectionsMaxRoughness : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingReflectionsMaxBounces : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingReflectionsSamplesPerPixel : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingReflectionsShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_TranslucencyType : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencyMaxRoughness : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencyRefractionRays : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencySamplesPerPixel : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencyShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencyRefraction : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingGI : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingGIMaxBounces : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingGISamplesPerPixel : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingMaxBounces : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingSamplesPerPixel : 1 = false;
    UPROPERTY()
    TObjectPtr<UCurveFloat> AutoExposureBiasCurve = nullptr;
    UPROPERTY()
    int32 RayTracingAOSamplesPerPixel = 1;
    UPROPERTY()
    int32 RayTracingGIMaxBounces = 0;
    UPROPERTY()
    int32 RayTracingGISamplesPerPixel = 0;
    UPROPERTY()
    float RayTracingReflectionsMaxRoughness = 0;
    UPROPERTY()
    int32 RayTracingReflectionsMaxBounces = 0;
    UPROPERTY()
    int32 RayTracingReflectionsSamplesPerPixel = 0;
    UPROPERTY()
    EReflectedAndRefractedRayTracedShadows RayTracingReflectionsShadows = EReflectedAndRefractedRayTracedShadows::Disabled;
    UPROPERTY()
    ETranslucencyType TranslucencyType = ETranslucencyType::Raster;
    UPROPERTY()
    float RayTracingTranslucencyMaxRoughness = 0.6f;
    UPROPERTY()
    int32 RayTracingTranslucencyRefractionRays = 3;
    UPROPERTY()
    int32 RayTracingTranslucencySamplesPerPixel = 1;
    UPROPERTY()
    EReflectedAndRefractedRayTracedShadows RayTracingTranslucencyShadows = EReflectedAndRefractedRayTracedShadows::Hard_shadows;
    UPROPERTY()
    uint8 RayTracingTranslucencyRefraction : 1 = true;
    UPROPERTY()
    int32 PathTracingMaxBounces = 32;
    UPROPERTY()
    int32 PathTracingSamplesPerPixel = 2048;
    UPROPERTY()
    uint8 bOverride_RayTracingAO : 1 = false;
    UPROPERTY()
    uint8 bOverride_MotionBlurTargetFPS : 1 = false;
    UPROPERTY()
    uint8 RayTracingAO : 1 = false;
    UPROPERTY()
    int32 MotionBlurTargetFPS = 30;
    UPROPERTY()
    uint8 bOverride_RayTracingReflectionsTranslucency : 1 = false;
    UPROPERTY()
    ERayTracingGlobalIlluminationType RayTracingGIType = ERayTracingGlobalIlluminationType::Disabled;
    UPROPERTY()
    uint8 RayTracingReflectionsTranslucency : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureCalibrationConstant_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureMeterMask : 1 = false;
    UPROPERTY()
    uint8 bOverride_AutoExposureApplyPhysicalCameraExposure : 1 = false;
    UPROPERTY()
    float AutoExposureBiasBackup = 0;
    UPROPERTY()
    uint8 bOverride_AutoExposureBiasBackup : 1 = false;
    UPROPERTY()
    uint8 AutoExposureApplyPhysicalCameraExposure : 1 = true;
    UPROPERTY()
    TObjectPtr<UTexture> AutoExposureMeterMask = nullptr;
    UPROPERTY()
    float AutoExposureCalibrationConstant_DEPRECATED = 16.0f;
    UPROPERTY()
    uint8 bOverride_ToneCurveAmount : 1 = false;
    UPROPERTY()
    uint8 bOverride_AmbientOcclusionTemporalBlendWeight : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingAOIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingAORadius : 1 = false;
    UPROPERTY()
    float ToneCurveAmount = 1.0;
    UPROPERTY()
    float AmbientOcclusionTemporalBlendWeight = 0.1f;
    UPROPERTY()
    float RayTracingAOIntensity = 1.0;
    UPROPERTY()
    float RayTracingAORadius = 200.0f;
    UPROPERTY()
    uint8 bOverride_TemperatureType : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingFilterWidth : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingEnableEmissive : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingMaxPathExposure : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingEnableDenoiser : 1 = false;
    UPROPERTY()
    TEnumAsByte<ETemperatureMethod> TemperatureType = ETemperatureMethod::TEMP_WhiteBalance;
    UPROPERTY()
    float PathTracingFilterWidth = 0;
    UPROPERTY()
    uint8 PathTracingEnableEmissive : 1 = false;
    UPROPERTY()
    float PathTracingMaxPathExposure = 0;
    UPROPERTY()
    uint8 PathTracingEnableDenoiser : 1 = true;
    UPROPERTY()
    uint8 bOverride_ColorCorrectionHighlightsMax : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionScatterDispersion : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureDetailStrength : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureBlurredLuminanceBlend : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureBlurredLuminanceKernelSizePercent : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureMiddleGreyBias : 1 = false;
    UPROPERTY()
    uint8 bOverride_GrainIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_GrainJitter_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainIntensityShadows : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainIntensityMidtones : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainIntensityHighlights : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainShadowsMax : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainHighlightsMin : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainHighlightsMax : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainTexture : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVDirectionalOcclusionIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVDirectionalOcclusionRadius_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVDiffuseOcclusionExponent_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVSpecularOcclusionExponent_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVDiffuseOcclusionIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVSpecularOcclusionIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVSize_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVSecondaryOcclusionIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVSecondaryBounceIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVGeometryVolumeBias_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVVplInjectionBias_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVEmissiveInjectionIntensity_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVFadeRange_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LPVDirectionalOcclusionFadeRange_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_ScreenPercentage_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_ReflectionMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenReflectionQuality : 1 = false;
    UPROPERTY()
    uint8 bOverride_ReflectionsType_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_DynamicGlobalIlluminationMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenSceneLightingQuality : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenSceneDetail : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenSceneViewDistance : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenSceneLightingUpdateSpeed : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenFinalGatherQuality : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenFinalGatherLightingUpdateSpeed : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenMaxTraceDistance : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenRayLightingMode : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingEnableReferenceDOF : 1 = false;
    UPROPERTY()
    float ColorCorrectionHighlightsMax = 1.0f;
    UPROPERTY()
    float BloomConvolutionScatterDispersion = 1.0f;
    UPROPERTY()
    TEnumAsByte<EDynamicGlobalIlluminationMethod::Type> DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
    UPROPERTY()
    float LumenSceneLightingQuality = 1;
    UPROPERTY()
    float LumenSceneDetail = 1.0f;
    UPROPERTY()
    float LumenSceneViewDistance = 20000.0f;
    UPROPERTY()
    float LumenSceneLightingUpdateSpeed = 1;
    UPROPERTY()
    float LumenFinalGatherQuality = 1;
    UPROPERTY()
    float LumenFinalGatherLightingUpdateSpeed = 1;
    UPROPERTY()
    float LumenMaxTraceDistance = 20000.0f;
    UPROPERTY()
    TEnumAsByte<EReflectionMethod::Type> ReflectionMethod = EReflectionMethod::Lumen;
    UPROPERTY()
    EReflectionsType ReflectionsType_DEPRECATED = EReflectionsType::RayTracing;
    UPROPERTY()
    float LumenReflectionQuality = 1;
    UPROPERTY()
    ELumenRayLightingModeOverride LumenRayLightingMode = ELumenRayLightingModeOverride::Default;
    UPROPERTY()
    float LocalExposureDetailStrength = 1.0f;
    UPROPERTY()
    float LocalExposureBlurredLuminanceBlend = 0.6f;
    UPROPERTY()
    float LocalExposureBlurredLuminanceKernelSizePercent = 50.0f;
    UPROPERTY()
    float LocalExposureMiddleGreyBias = 0.0f;
    UPROPERTY()
    float GrainJitter_DEPRECATED = 0.0f;
    UPROPERTY()
    float GrainIntensity_DEPRECATED = 0.0f;
    UPROPERTY()
    float FilmGrainIntensity = 0.0f;
    UPROPERTY()
    float FilmGrainIntensityShadows = 1.0f;
    UPROPERTY()
    float FilmGrainIntensityMidtones = 1.0f;
    UPROPERTY()
    float FilmGrainIntensityHighlights = 1.0f;
    UPROPERTY()
    float FilmGrainShadowsMax = 0.09f;
    UPROPERTY()
    float FilmGrainHighlightsMin = 0.5f;
    UPROPERTY()
    float FilmGrainHighlightsMax = 1.0f;
    UPROPERTY()
    TObjectPtr<UTexture2D> FilmGrainTexture = nullptr;
    UPROPERTY()
    float LPVIntensity_DEPRECATED = 0;
    UPROPERTY()
    float LPVVplInjectionBias_DEPRECATED = 0;
    UPROPERTY()
    float LPVSize_DEPRECATED = 0;
    UPROPERTY()
    float LPVSecondaryOcclusionIntensity_DEPRECATED = 0;
    UPROPERTY()
    float LPVSecondaryBounceIntensity_DEPRECATED = 0;
    UPROPERTY()
    float LPVGeometryVolumeBias_DEPRECATED = 0;
    UPROPERTY()
    float LPVEmissiveInjectionIntensity_DEPRECATED = 0;
    UPROPERTY()
    float LPVDirectionalOcclusionIntensity_DEPRECATED = 0;
    UPROPERTY()
    float LPVDirectionalOcclusionRadius_DEPRECATED = 0;
    UPROPERTY()
    float LPVDiffuseOcclusionExponent_DEPRECATED = 0;
    UPROPERTY()
    float LPVSpecularOcclusionExponent_DEPRECATED = 0;
    UPROPERTY()
    float LPVDiffuseOcclusionIntensity_DEPRECATED = 0;
    UPROPERTY()
    float LPVSpecularOcclusionIntensity_DEPRECATED = 0;
    UPROPERTY()
    uint8 PathTracingEnableReferenceDOF : 1 = false;
    UPROPERTY()
    float LPVFadeRange_DEPRECATED = 0;
    UPROPERTY()
    float LPVDirectionalOcclusionFadeRange_DEPRECATED = 0;
    UPROPERTY()
    float ScreenPercentage_DEPRECATED = 100.0f;
    UPROPERTY()
    uint8 bOverride_LocalExposureContrastScale_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureHighlightContrastScale : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureShadowContrastScale : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldSqueezeFactor : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenDiffuseColorBoost : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenSkylightLeaking : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenFullSkylightLeakingDistance : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenFrontLayerTranslucencyReflections : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenSurfaceCacheResolution : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingEnableReferenceAtmosphere : 1 = false;
    UPROPERTY()
    float LumenDiffuseColorBoost = 1.0f;
    UPROPERTY()
    float LumenSkylightLeaking = 0.0f;
    UPROPERTY()
    float LumenFullSkylightLeakingDistance = 1000.0f;
    UPROPERTY()
    float LumenSurfaceCacheResolution = 1.0f;
    UPROPERTY()
    uint8 LumenFrontLayerTranslucencyReflections : 1 = false;
    UPROPERTY()
    float LocalExposureContrastScale_DEPRECATED = 1.0f;
    UPROPERTY()
    float LocalExposureHighlightContrastScale = 1.0f;
    UPROPERTY()
    float LocalExposureShadowContrastScale = 1.0f;
    UPROPERTY()
    float DepthOfFieldSqueezeFactor = 1.0f;
    UPROPERTY()
    uint8 PathTracingEnableReferenceAtmosphere : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeEmissive : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeIndirectEmissive : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeDiffuse : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeIndirectDiffuse : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeSpecular : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeIndirectSpecular : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeVolume : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingIncludeIndirectVolume : 1 = false;
    UPROPERTY()
    uint8 PathTracingIncludeEmissive : 1 = true;
    UPROPERTY()
    uint8 PathTracingIncludeIndirectEmissive : 1 = false;
    UPROPERTY()
    uint8 PathTracingIncludeDiffuse : 1 = true;
    UPROPERTY()
    uint8 PathTracingIncludeIndirectDiffuse : 1 = true;
    UPROPERTY()
    uint8 PathTracingIncludeSpecular : 1 = true;
    UPROPERTY()
    uint8 PathTracingIncludeIndirectSpecular : 1 = true;
    UPROPERTY()
    uint8 PathTracingIncludeVolume : 1 = true;
    UPROPERTY()
    uint8 PathTracingIncludeIndirectVolume : 1 = true;
    UPROPERTY()
    uint8 bOverride_Sharpen : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenMaxReflectionBounces : 1 = false;
    UPROPERTY()
    int32 LumenMaxReflectionBounces = 1;
    UPROPERTY()
    float Sharpen = 0.0f;
    UPROPERTY()
    uint8 bOverride_LocalExposureHighlightContrastCurve : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureShadowContrastCurve : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureHighlightThreshold : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureShadowThreshold : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldUseHairDepth : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenFinalGatherScreenTraces : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenReflectionsScreenTraces : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenMaxRoughnessToTraceReflections : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenMaxRefractionBounces : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingEnableEmissiveMaterials : 1 = false;
    UPROPERTY()
    uint8 LumenFinalGatherScreenTraces : 1 = true;
    UPROPERTY()
    uint8 LumenReflectionsScreenTraces : 1 = true;
    UPROPERTY()
    float LumenMaxRoughnessToTraceReflections = 0.4f;
    UPROPERTY()
    int32 LumenMaxRefractionBounces = 0;
    UPROPERTY()
    TObjectPtr<UCurveFloat> LocalExposureHighlightContrastCurve = nullptr;
    UPROPERTY()
    TObjectPtr<UCurveFloat> LocalExposureShadowContrastCurve = nullptr;
    UPROPERTY()
    float LocalExposureHighlightThreshold = 0;
    UPROPERTY()
    float LocalExposureShadowThreshold = 0;
    UPROPERTY()
    uint8 DepthOfFieldUseHairDepth : 1 = false;
    UPROPERTY()
    uint8 PathTracingEnableEmissiveMaterials : 1 = true;
    UPROPERTY()
    uint8 bOverride_bMegaLights : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_UserFlags : 1 = false;
    UPROPERTY()
    uint8 bOverride_PathTracingMaxPathIntensity : 1 = false;
    UPROPERTY()
    uint8 bMegaLights : 1 = true;
    UPROPERTY()
    ELocalExposureMethod LocalExposureMethod = ELocalExposureMethod::Bilateral;
    UPROPERTY()
    float PathTracingMaxPathIntensity = 24.0f;
    UPROPERTY()
    int32 UserFlags = 0;
    UPROPERTY()
    uint8 bOverride_LocalExposureHighlightThresholdStrength : 1 = false;
    UPROPERTY()
    uint8 bOverride_LocalExposureShadowThresholdStrength : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldPetzvalBokeh : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldPetzvalBokehFalloff : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldPetzvalExclusionBoxExtents : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldPetzvalExclusionBoxRadius : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldAspectRatioScalar : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldMatteBoxFlags : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldBarrelRadius : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldBarrelLength : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencyMaxPrimaryHitEvents : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencyMaxSecondaryHitEvents : 1 = false;
    UPROPERTY()
    uint8 bOverride_RayTracingTranslucencyUseRayTracedRefraction : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenSkylightLeakingTint : 1 = false;
    UPROPERTY()
    FLinearColor LumenSkylightLeakingTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    float LocalExposureHighlightThresholdStrength = 1.0f;
    UPROPERTY()
    float LocalExposureShadowThresholdStrength = 1.0f;
    UPROPERTY()
    float DepthOfFieldPetzvalBokeh = 0;
    UPROPERTY()
    float DepthOfFieldPetzvalBokehFalloff = 1.0f;
    UPROPERTY()
    FVector2f DepthOfFieldPetzvalExclusionBoxExtents = FVector2f::ZeroVector;
    UPROPERTY()
    float DepthOfFieldPetzvalExclusionBoxRadius = 0.0f;
    UPROPERTY()
    float DepthOfFieldAspectRatioScalar = 1.0f;
    UPROPERTY()
    float DepthOfFieldBarrelRadius = 5;
    UPROPERTY()
    float DepthOfFieldBarrelLength = 0;
    UPROPERTY()
    FMatteBoxFlag DepthOfFieldMatteBoxFlags[3];
    UPROPERTY()
    int32 RayTracingTranslucencyMaxPrimaryHitEvents = 4;
    UPROPERTY()
    int32 RayTracingTranslucencyMaxSecondaryHitEvents = 2;
    UPROPERTY()
    uint8 RayTracingTranslucencyUseRayTracedRefraction : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomGaussianIntensity : 1 = false;
    UPROPERTY()
    uint8 bOverride_BloomConvolutionIntensity : 1 = false;
    UPROPERTY()
    float BloomGaussianIntensity = 1.0f;
    UPROPERTY()
    float BloomConvolutionIntensity = 1.0f;
    UPROPERTY()
    uint8 bOverride_TonemappingMethod : 1 = false;
    UPROPERTY()
    uint8 bOverride_MegaLightsFrontLayerTranslucency : 1 = false;
    UPROPERTY()
    uint8 bOverride_VignetteType : 1 = false;
    UPROPERTY()
    uint8 bOverride_VignetteCenter : 1 = false;
    UPROPERTY()
    uint8 bOverride_VignetteSize : 1 = false;
    UPROPERTY()
    uint8 bOverride_VignetteSoftness : 1 = false;
    UPROPERTY()
    uint8 bOverride_VignetteTexture : 1 = false;
    UPROPERTY()
    uint8 bOverride_FilmGrainScale : 1 = false;
    UPROPERTY()
    uint8 bOverride_DepthOfFieldEnabled : 1 = false;
    UPROPERTY()
    uint8 bOverride_LumenAmbientOcclusionIntensity : 1 = false;
    UPROPERTY()
    ETonemappingMethod TonemappingMethod = ETonemappingMethod::Default;
    UPROPERTY()
    float LumenAmbientOcclusionIntensity = 1.0f;
    UPROPERTY()
    uint8 MegaLightsFrontLayerTranslucency : 1 = false;
    UPROPERTY()
    EVignetteType VignetteType = EVignetteType::CosineFourthLaw;
    UPROPERTY()
    FVector2f VignetteCenter = FVector2f(0.5f, 0.5f);
    UPROPERTY()
    FVector2f VignetteSize = FVector2f(1.0f, 1.0f);
    UPROPERTY()
    float VignetteSoftness = 5.0f;
    UPROPERTY()
    TObjectPtr<UTexture> VignetteTexture = nullptr;
    UPROPERTY()
    FVector2f FilmGrainScale = FVector2f{1.0f, 1.0f};
    UPROPERTY()
    bool DepthOfFieldEnabled = true;
};

}
