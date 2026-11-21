#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "GroomAssetRendering.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

USTRUCT()
struct FHairGeometrySettings {
    GENERATED_BODY()
    UPROPERTY()
    float HairWidth = 0.01f;
    UPROPERTY()
    bool HairWidth_Override = false;
    UPROPERTY()
    float HairRootScale = 1.0f;
    UPROPERTY()
    float HairTipScale = 1.0f;
};

USTRUCT()
struct FHairShadowSettings {
    GENERATED_BODY()
    UPROPERTY()
    float HairShadowDensity = 1.0f;
    UPROPERTY()
    float HairRaytracingRadiusScale = 1.0f;
    UPROPERTY()
    bool bUseHairRaytracingGeometry = false;
    UPROPERTY()
    bool bVoxelize = true;
};

USTRUCT()
struct FHairAdvancedRenderingSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bUseStableRasterization = false;
    UPROPERTY()
    bool bScatterSceneLighting = false;
};

USTRUCT()
struct FHairGroupsRendering {
    GENERATED_BODY()
    UPROPERTY()
    FName MaterialSlotName;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    FHairGeometrySettings GeometrySettings;
    UPROPERTY()
    FHairShadowSettings ShadowSettings;
    UPROPERTY()
    FHairAdvancedRenderingSettings AdvancedSettings;
};

}
