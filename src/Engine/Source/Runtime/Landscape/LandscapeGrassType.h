#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/SceneTypes.h"

#include "LandscapeGrassType.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UStaticMesh;

UENUM()
enum class EGrassScaling : uint8 {
    Uniform,
    Free,
    LockXY,
};

USTRUCT()
struct FGrassVariety {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UStaticMesh> GrassMesh = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;
    UPROPERTY()
    FPerPlatformFloat GrassDensity{400.0f};
    UPROPERTY()
    FPerQualityLevelFloat GrassDensityQuality{400.0f};
    UPROPERTY()
    bool bUseGrid = true;
    UPROPERTY()
    float PlacementJitter = 1.0f;
    UPROPERTY()
    FPerPlatformInt StartCullDistance{10000};
    UPROPERTY()
    FPerQualityLevelInt StartCullDistanceQuality{10000};
    UPROPERTY()
    FPerPlatformInt EndCullDistance{10000};
    UPROPERTY()
    FPerQualityLevelInt EndCullDistanceQuality{10000};
    UPROPERTY()
    int32 MinLOD = -1;
    UPROPERTY()
    FFloatInterval AllowedDensityRange{.Min = 0.0f, .Max = 1.0f};
    UPROPERTY()
    EGrassScaling Scaling = EGrassScaling::Uniform;
    UPROPERTY()
    FFloatInterval ScaleX{.Min = 1.0f, .Max = 1.0f};
    UPROPERTY()
    FFloatInterval ScaleY{.Min = 1.0f, .Max = 1.0f};
    UPROPERTY()
    FFloatInterval ScaleZ{.Min = 1.0f, .Max = 1.0f};
    UPROPERTY()
    bool bWeightAttenuatesMaxScale = false;
    UPROPERTY()
    float MaxScaleWeightAttenuation = 0.5f;
    UPROPERTY()
    bool RandomRotation = true;
    UPROPERTY()
    bool AlignToSurface = true;
    UPROPERTY()
    bool bAlignToTriangleNormals = true;
    UPROPERTY()
    bool bUseLandscapeLightmap = false;
    UPROPERTY()
    FLightingChannels LightingChannels;
    UPROPERTY()
    bool bReceivesDecals = true;
    UPROPERTY()
    bool bAffectDistanceFieldLighting = false;
    UPROPERTY()
    bool bCastDynamicShadow = true;
    UPROPERTY()
    bool bCastContactShadow = true;
    UPROPERTY()
    bool bKeepInstanceBufferCPUCopy = false;
    UPROPERTY()
    uint32 InstanceWorldPositionOffsetDisableDistance = 0;
    UPROPERTY()
    EShadowCacheInvalidationBehavior ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Auto;
};

UCLASS()
class ULandscapeGrassType : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FGrassVariety> GrassVarieties;
    UPROPERTY()
    uint32 bEnableDensityScaling : 1 = true;
    UPROPERTY()
    uint32 StateHash = 0;
    UPROPERTY()
    TObjectPtr<UStaticMesh> GrassMesh_DEPRECATED = nullptr;
    UPROPERTY()
    float GrassDensity_DEPRECATED = 400.0f;
    UPROPERTY()
    float PlacementJitter_DEPRECATED = 1.0f;
    UPROPERTY()
    int32 StartCullDistance_DEPRECATED = 10000;
    UPROPERTY()
    int32 EndCullDistance_DEPRECATED = 10000;
    UPROPERTY()
    bool RandomRotation_DEPRECATED = true;
    UPROPERTY()
    bool AlignToSurface_DEPRECATED = true;
};

}
