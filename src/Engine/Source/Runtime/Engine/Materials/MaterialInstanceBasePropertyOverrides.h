#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "MaterialInstanceBasePropertyOverrides.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMaterialInstanceBasePropertyOverrides {
    GENERATED_BODY()
    UPROPERTY()
    bool bOverride_OpacityMaskClipValue = false;
    UPROPERTY()
    bool bOverride_BlendMode = false;
    UPROPERTY()
    bool bOverride_ShadingModel = false;
    UPROPERTY()
    bool bOverride_TwoSided = false;
    UPROPERTY()
    float OpacityMaskClipValue = 0.333333f;
    UPROPERTY()
    TEnumAsByte<EBlendMode> BlendMode = BLEND_Opaque;
    UPROPERTY()
    TEnumAsByte<EMaterialShadingModel> ShadingModel = MSM_DefaultLit;
    UPROPERTY()
    bool TwoSided = false;
    UPROPERTY()
    bool bOverride_DitheredLODTransition = false;
    UPROPERTY()
    bool DitheredLODTransition = false;
    UPROPERTY()
    bool bOverride_CastDynamicShadowAsMasked = false;
    UPROPERTY()
    bool bCastDynamicShadowAsMasked = false;
    UPROPERTY()
    bool bOverride_OutputTranslucentVelocity = false;
    UPROPERTY()
    bool bOutputTranslucentVelocity = false;
    UPROPERTY()
    bool bOverride_bIsThinSurface = false;
    UPROPERTY()
    bool bOverride_MaxWorldPositionOffsetDisplacement = false;
    UPROPERTY()
    bool bIsThinSurface = false;
    UPROPERTY()
    float MaxWorldPositionOffsetDisplacement = 0;
    UPROPERTY()
    bool bOverride_DisplacementScaling = false;
    UPROPERTY()
    FDisplacementScaling DisplacementScaling;
    UPROPERTY()
    bool bOverride_bHasPixelAnimation = false;
    UPROPERTY()
    bool bOverride_bEnableTessellation = false;
    UPROPERTY()
    bool bHasPixelAnimation = false;
    UPROPERTY()
    bool bEnableTessellation = false;
    UPROPERTY()
    bool bOverride_bEnableDisplacementFade = false;
    UPROPERTY()
    bool bOverride_DisplacementFadeRange = false;
    UPROPERTY()
    bool bEnableDisplacementFade = false;
    UPROPERTY()
    FDisplacementFadeRange DisplacementFadeRange;
    UPROPERTY()
    bool bOverride_CompatibleWithLumenCardSharing = false;
    UPROPERTY()
    bool bCompatibleWithLumenCardSharing = false;
    UPROPERTY()
    uint32 bOverride_UsageFlags = 0;
    UPROPERTY()
    uint32 UsageFlags = 0;
};

}
