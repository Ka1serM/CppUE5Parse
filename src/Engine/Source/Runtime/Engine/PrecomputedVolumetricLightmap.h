#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/PixelFormat.h"

namespace ue5parse::runtime {

class FVolumetricLightmapDataLayer {
public:
    TArray<uint8> Data;
    EPixelFormat Format = EPixelFormat::PF_Unknown;

    friend FArchive& operator<<(FArchive& Ar, FVolumetricLightmapDataLayer& Layer);
};

class FVolumetricLightmapBrickData {
public:
    FVolumetricLightmapDataLayer AmbientVector;
    FVolumetricLightmapDataLayer SHCoefficients[6];
    FVolumetricLightmapDataLayer SkyBentNormal;
    FVolumetricLightmapDataLayer DirectionalLightShadowing;
};

class FPrecomputedVolumetricLightmapData {
public:
    FBox Bounds;
    FIntVector IndirectionTextureDimensions;
    FVolumetricLightmapDataLayer IndirectionTexture;
    int32 BrickSize = 0;
    FIntVector BrickDataDimensions;
    FVolumetricLightmapBrickData BrickData;
    TArray<FIntVector> SubLevelBrickPositions;
    TArray<FColor> IndirectionTextureOriginalValues;

    friend FArchive& operator<<(FArchive& Ar, FPrecomputedVolumetricLightmapData& Volume);
};

FArchive& operator<<(FArchive& Ar, std::unique_ptr<FPrecomputedVolumetricLightmapData>& Volume);

}
