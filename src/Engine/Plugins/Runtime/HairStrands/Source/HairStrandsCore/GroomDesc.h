#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "GroomDesc.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FHairGroupDesc {
    GENERATED_BODY()

    UPROPERTY()
    int32 HairCount = 0;
    UPROPERTY()
    int32 GuideCount = 0;
    UPROPERTY()
    float HairLength = 0.0f;
    UPROPERTY()
    float HairWidth = 0.1f;
    UPROPERTY()
    bool HairWidth_Override = false;
    UPROPERTY()
    float HairRootScale = 1.0f;
    UPROPERTY()
    bool HairRootScale_Override = false;
    UPROPERTY()
    float HairTipScale = 1.0f;
    UPROPERTY()
    bool HairTipScale_Override = false;
    UPROPERTY()
    float HairClipLength = 0.0f;
    UPROPERTY()
    bool HairClipLength_Override = false;
    UPROPERTY()
    float HairShadowDensity = 1.0f;
    UPROPERTY()
    bool HairShadowDensity_Override = false;
    UPROPERTY()
    float HairRaytracingRadiusScale = 1.0f;
    UPROPERTY()
    bool HairRaytracingRadiusScale_Override = false;
    UPROPERTY()
    bool bUseHairRaytracingGeometry = false;
    UPROPERTY()
    bool bUseHairRaytracingGeometry_Override = false;
    UPROPERTY()
    float LODBias = 0.0f;
    UPROPERTY()
    bool bUseStableRasterization = false;
    UPROPERTY()
    bool bUseStableRasterization_Override = false;
    UPROPERTY()
    bool bScatterSceneLighting = false;
    UPROPERTY()
    bool bScatterSceneLighting_Override = false;
    UPROPERTY()
    bool bSupportVoxelization = true;
    UPROPERTY()
    bool bSupportVoxelization_Override = false;
    UPROPERTY()
    int32 LODForcedIndex = -1;
    UPROPERTY()
    float HairLengthScale = 1.0f;
    UPROPERTY()
    bool HairLengthScale_Override = false;
};

}
