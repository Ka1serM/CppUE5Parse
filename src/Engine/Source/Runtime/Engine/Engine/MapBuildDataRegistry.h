#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Math/Float16.h"
#include "Engine/Source/Runtime/Core/Math/Matrix.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/LightMap.h"
#include "Engine/Source/Runtime/Engine/PrecomputedLightVolume.h"
#include "Engine/Source/Runtime/Engine/PrecomputedVolumetricLightmap.h"
#include "Engine/Source/Runtime/Engine/ShadowMap.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/StaticLightingData/VolumetricLightmapGrid.h"

#include "MapBuildDataRegistry.generated.h"

namespace ue5parse::runtime {

class UTextureCube;

struct FPerInstanceLightmapData {
    FVector2f LightmapUVBias;
    FVector2f ShadowmapUVBias;
};

class FMeshMapBuildData {
public:
    FLightMapRef LightMap;
    FShadowMapRef ShadowMap;
    TArray<FGuid> IrrelevantLights;
    TArray<FPerInstanceLightmapData> PerInstanceLightmapData;

    friend FArchive& operator<<(FArchive& Ar, FMeshMapBuildData& MeshMapBuildData);
};

class FStaticShadowDepthMapData {
public:
    FMatrix WorldToLight;
    int32 ShadowMapSizeX = 0;
    int32 ShadowMapSizeY = 0;
    TArray<FFloat16> DepthSamples;

    friend FArchive& operator<<(FArchive& Ar, FStaticShadowDepthMapData& ShadowMap);
};

class FLightComponentMapBuildData {
public:
    int32 ShadowMapChannel = INDEX_NONE;
    FStaticShadowDepthMapData DepthMap;

    friend FArchive& operator<<(FArchive& Ar, FLightComponentMapBuildData& LightBuildData);
};

class FReflectionCaptureMapBuildData {
public:
    int32 CubemapSize = 0;
    float AverageBrightness = 0.0f;
    float Brightness = 1.0f;
    TArray<uint8> FullHDRCapturedData;
    TArray<uint8> EncodedHDRCapturedData;
    TObjectPtr<UTextureCube> EncodedCaptureData;

    friend FArchive& operator<<(FArchive& Ar, FReflectionCaptureMapBuildData& ReflectionCaptureMapBuildData);
};

class FSkyAtmosphereMapBuildData {
public:
    friend FArchive& operator<<(FArchive& Ar, FSkyAtmosphereMapBuildData&) { return Ar; }
};

UCLASS()
class UMapBuildDataRegistry : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<ELightingBuildQuality> LevelLightingQuality = Quality_MAX;

    void Serialize(FArchive& Ar) override;

private:
    TMap<FGuid, FMeshMapBuildData> MeshBuildData;
    TMap<FGuid, std::unique_ptr<FPrecomputedLightVolumeData>> LevelPrecomputedLightVolumeBuildData;
    TMap<FGuid, std::unique_ptr<FPrecomputedVolumetricLightmapData>> LevelPrecomputedVolumetricLightmapBuildData;
    TMap<FGuid, FLightComponentMapBuildData> LightBuildData;
    TMap<FGuid, FReflectionCaptureMapBuildData> ReflectionCaptureBuildData;
    TMap<FGuid, FSkyAtmosphereMapBuildData> SkyAtmosphereBuildData;
    std::unique_ptr<FVolumetricLightMapGridDesc> VolumetricLightMapGridDesc;
};

}
