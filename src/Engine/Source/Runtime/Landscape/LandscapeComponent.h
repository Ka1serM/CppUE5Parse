#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/StreamableRenderAsset.h"

#include "LandscapeComponent.generated.h"

namespace ue5parse::runtime {

class ULandscapeGrassType;
class ULandscapeHeightfieldCollisionComponent;
class ULandscapeLayerInfoObject;
class UMaterialInstanceConstant;
class UMaterialInterface;
class UTexture2D;
class UTexture2DArray;

USTRUCT()
struct FWeightmapLayerAllocationInfo {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<ULandscapeLayerInfoObject> LayerInfo = nullptr;
    UPROPERTY()
    uint8 WeightmapTextureIndex = 255;
    UPROPERTY()
    uint8 WeightmapTextureChannel = 255;
    UPROPERTY()
    uint8 GrassMapChannelIndex = 255;
    UPROPERTY()
    FName LayerName_DEPRECATED;
};

USTRUCT()
struct FLandscapeComponentMaterialOverride {
    GENERATED_BODY()
    UPROPERTY()
    FPerPlatformInt LODIndex{0};
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
};

USTRUCT()
struct FLandscapePerLODMaterialOverride {
    GENERATED_BODY()
    UPROPERTY()
    int32 LODIndex = 0;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
};

struct FLandscapeComponentGrassData {
    TArray<uint16> HeightData;
    TMap<TObjectPtr<ULandscapeGrassType>, TArray<uint8>> WeightData;
    int32 NumElements = -1;
    TMap<TObjectPtr<ULandscapeGrassType>, int32> GrassTypeWeightOffsets;
    TMap<FName, int32> WeightOffsets;
    TArray<uint8> HeightWeightData;

    friend FArchive& operator<<(FArchive& Ar, FLandscapeComponentGrassData& Data);
};

struct FLandscapeComponentDerivedData {
    TArray<uint8> CompressedLandscapeData;
    TArray<FByteBulkData> StreamingLODDataArray;

    void Serialize(FArchive& Ar);
};

UCLASS()
class ULandscapeLODStreamingProxy : public UStreamableRenderAsset {
    GENERATED_BODY()
};

UCLASS()
class ULandscapeComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit ULandscapeComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    int32 SectionBaseX = 0;
    UPROPERTY()
    int32 SectionBaseY = 0;
    UPROPERTY()
    int32 ComponentSizeQuads = 0;
    UPROPERTY()
    int32 SubsectionSizeQuads = 0;
    UPROPERTY()
    int32 NumSubsections = 0;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverrideMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OverrideHoleMaterial = nullptr;
    UPROPERTY()
    TArray<FLandscapeComponentMaterialOverride> OverrideMaterials;
    UPROPERTY()
    TArray<FLandscapePerLODMaterialOverride> PerLODOverrideMaterials;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceConstant> MaterialInstance = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceConstant>> MaterialInstances;
    UPROPERTY()
    TArray<int8> LODIndexToMaterialIndex;
    UPROPERTY()
    TArray<int8> MaterialIndexToDisabledTessellationMaterial;
    UPROPERTY()
    TObjectPtr<UTexture2D> XYOffsetmapTexture = nullptr;
    UPROPERTY()
    FVector4 WeightmapScaleBias{0.0, 0.0, 0.0, 1.0};
    UPROPERTY()
    float WeightmapSubsectionOffset = 0.0f;
    UPROPERTY()
    FVector4 HeightmapScaleBias{0.0, 0.0, 0.0, 1.0};
    UPROPERTY()
    FBox CachedLocalBox;
    UPROPERTY()
    FBoxSphereBounds CachedBoxSphereBounds_DEPRECATED;
    UPROPERTY()
    TArray<double> MipToMipMaxDeltas;
    UPROPERTY()
    TLazyObjectPtr<ULandscapeHeightfieldCollisionComponent> CollisionComponent;
    UPROPERTY()
    TObjectPtr<ULandscapeHeightfieldCollisionComponent> CollisionComponentRef = nullptr;
    UPROPERTY()
    bool bUserTriggeredChangeRequested = false;
    UPROPERTY()
    bool bNaniteActive = false;
    UPROPERTY()
    TObjectPtr<UTexture2D> HeightmapTexture = nullptr;
    UPROPERTY()
    TArray<FWeightmapLayerAllocationInfo> WeightmapLayerAllocations;
    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> WeightmapTextures;
    UPROPERTY()
    TArray<TObjectPtr<ULandscapeGrassType>> GrassTypes;
    UPROPERTY()
    TMap<FName, TObjectPtr<ULandscapeGrassType>> NamedGrassTypes;
    UPROPERTY()
    FGuid MapBuildDataId;
    UPROPERTY()
    TArray<FGuid> IrrelevantLights;
    UPROPERTY()
    int32 CollisionMipLevel = 0;
    UPROPERTY()
    int32 SimpleCollisionMipLevel = 0;
    UPROPERTY()
    float NegativeZBoundsExtension = 0.0f;
    UPROPERTY()
    float PositiveZBoundsExtension = 0.0f;
    UPROPERTY()
    float StaticLightingResolution = 0.0f;
    UPROPERTY()
    int32 ForcedLOD = -1;
    UPROPERTY()
    int32 LODBias = 0;
    UPROPERTY()
    FGuid StateId;
    UPROPERTY()
    FGuid BakedTextureMaterialGuid;
    UPROPERTY()
    FGuid LastBakedTextureMaterialGuid;
    UPROPERTY()
    TObjectPtr<UTexture2D> GIBakedBaseColorTexture = nullptr;
    UPROPERTY()
    uint8 MobileBlendableLayerMask = 0;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MobileMaterialInterface = nullptr;
    UPROPERTY()
    TObjectPtr<UTexture2D> MobileWeightNormalmapTexture = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> MobileMaterialInterfaces;
    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> MobileWeightmapTextures;
    UPROPERTY()
    TObjectPtr<UTexture2DArray> MobileWeightmapTextureArray = nullptr;
    UPROPERTY()
    TArray<FWeightmapLayerAllocationInfo> MobileWeightmapLayerAllocations;
    UPROPERTY()
    TObjectPtr<ULandscapeLODStreamingProxy> LODStreamingProxy = nullptr;

    FLandscapeComponentGrassData GrassData;
    FLandscapeComponentDerivedData PlatformData;

    void Serialize(FArchive& Ar) override;

private:
    void SerializeCookedPlatformData(FArchive& Ar);
};

}
