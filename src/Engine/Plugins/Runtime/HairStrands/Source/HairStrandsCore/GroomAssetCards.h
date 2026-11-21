#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "GroomAssetCards.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UStaticMesh;
class UTexture2D;

UENUM()
enum class EHairCardsSourceType : uint8 { Procedural, Imported };

UENUM()
enum class EHairCardsGuideType : uint8 { Generated, GuideBased };

UENUM()
enum class EHairTextureLayout : uint8 { Layout0, Layout1, Layout2, Layout3 };

UENUM()
enum class EHairCardsClusterType : uint8 { Low, High };

UENUM()
enum class EHairCardsGenerationType : uint8 { CardsCount, UseGuides };

USTRUCT()
struct FHairCardsClusterSettings {
    GENERATED_BODY()
    UPROPERTY()
    float ClusterDecimation = 0.1f;
    UPROPERTY()
    EHairCardsClusterType Type = EHairCardsClusterType::High;
    UPROPERTY()
    bool bUseGuide = true;
};

USTRUCT()
struct FHairCardsGeometrySettings {
    GENERATED_BODY()
    UPROPERTY()
    EHairCardsGenerationType GenerationType = EHairCardsGenerationType::UseGuides;
    UPROPERTY()
    int32 CardsCount = 5000;
    UPROPERTY()
    EHairCardsClusterType ClusterType = EHairCardsClusterType::High;
    UPROPERTY()
    float MinSegmentLength = 1.0f;
    UPROPERTY()
    float AngularThreshold = 5.0f;
    UPROPERTY()
    float MinCardsLength = 0.0f;
    UPROPERTY()
    float MaxCardsLength = 0.0f;
};

USTRUCT()
struct FHairCardsTextureSettings {
    GENERATED_BODY()
    UPROPERTY()
    int32 AtlasMaxResolution = 2048;
    UPROPERTY()
    int32 PixelPerCentimeters = 60;
    UPROPERTY()
    int32 LengthTextureCount = 10;
    UPROPERTY()
    int32 DensityTextureCount = 1;
};

USTRUCT()
struct FHairGroupsProceduralCards {
    GENERATED_BODY()
    UPROPERTY()
    FHairCardsClusterSettings ClusterSettings;
    UPROPERTY()
    FHairCardsGeometrySettings GeometrySettings;
    UPROPERTY()
    FHairCardsTextureSettings TextureSettings;
    UPROPERTY()
    int32 Version = 0;
};

USTRUCT()
struct FHairGroupCardsInfo {
    GENERATED_BODY()
    UPROPERTY()
    int32 NumCards = 0;
    UPROPERTY()
    int32 NumCardVertices = 0;
};

USTRUCT()
struct FHairGroupCardsTextures {
    GENERATED_BODY()
    UPROPERTY()
    EHairTextureLayout Layout = EHairTextureLayout::Layout0;
    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> Textures;
    UPROPERTY()
    TObjectPtr<UTexture2D> DepthTexture_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UTexture2D> CoverageTexture_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UTexture2D> TangentTexture_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UTexture2D> AttributeTexture_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UTexture2D> AuxilaryDataTexture_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UTexture2D> MaterialTexture_DEPRECATED = nullptr;
};

USTRUCT()
struct FHairGroupsCardsSourceDescription {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    FName MaterialSlotName;
    UPROPERTY()
    EHairCardsSourceType SourceType_DEPRECATED = EHairCardsSourceType::Imported;
    UPROPERTY()
    TObjectPtr<UStaticMesh> ProceduralMesh_DEPRECATED = nullptr;
    UPROPERTY()
    FString ProceduralMeshKey;
    UPROPERTY()
    TObjectPtr<UStaticMesh> ImportedMesh = nullptr;
    UPROPERTY()
    bool bInvertUV = false;
    UPROPERTY()
    EHairCardsGuideType GuideType = EHairCardsGuideType::Generated;
    UPROPERTY()
    FHairGroupsProceduralCards ProceduralSettings;
    UPROPERTY()
    FHairGroupCardsTextures Textures;
    UPROPERTY()
    int32 GroupIndex = 0;
    UPROPERTY()
    int32 LODIndex = -1;
    UPROPERTY()
    FHairGroupCardsInfo CardsInfo;
    UPROPERTY()
    FString ImportedMeshKey;
};

}
