#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavDataGatheringMode.h"
#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/ActorPartition/PartitionActor.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodyInstance.h"
#include "Engine/Source/Runtime/Engine/SceneTypes.h"
#include "Engine/Source/Runtime/Engine/VT/RuntimeVirtualTextureEnum.h"
#include "Engine/Source/Runtime/Landscape/LandscapeComponent.h"

#include "LandscapeProxy.generated.h"

namespace ue5parse::runtime {

class ULandscapeGrassType;
class ULandscapeHeightfieldCollisionComponent;
class ULandscapeLayerInfoObject;
class ULandscapeNaniteComponent;
class ULandscapeSplinesComponent;
class ULandscapeWeightmapUsage;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UPhysicalMaterial;
class URuntimeVirtualTexture;
class UTexture2D;

namespace ELandscapeLODFalloff {

UENUM()
enum Type : int32 { Linear, SquareRoot };

}

USTRUCT()
struct FLandscapeGrassName {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;

    bool operator==(const FLandscapeGrassName& Other) const { return Name == Other.Name; }
    friend uint32 GetTypeHash(const FLandscapeGrassName& GrassName) { return GetTypeHash(GrassName.Name); }
};

USTRUCT()
struct FLandscapeTargetLayerSettings {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<ULandscapeLayerInfoObject> LayerInfoObj = nullptr;
};

USTRUCT()
struct FLandscapeProxyMaterialOverride {
    GENERATED_BODY()
    UPROPERTY()
    FPerPlatformInt LODIndex{0};
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
};

UCLASS(Abstract)
class ALandscapeProxy : public APartitionActor {
    GENERATED_BODY()
public:
    explicit ALandscapeProxy(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<ULandscapeSplinesComponent> SplineComponent = nullptr;
    UPROPERTY()
    FGuid LandscapeGuid;
    UPROPERTY()
    FGuid OriginalLandscapeGuid;
    UPROPERTY()
    FIntPoint LandscapeSectionOffset{0, 0};
    UPROPERTY()
    FIntPoint SectionBase{0, 0};
    UPROPERTY()
    bool bEnableNanite = false;
    UPROPERTY()
    TArray<FLandscapePerLODMaterialOverride> PerLODOverrideMaterials;
    UPROPERTY()
    int32 NaniteLODIndex = 0;
    UPROPERTY()
    bool bNaniteSkirtEnabled = false;
    UPROPERTY()
    float NaniteSkirtDepth = 0.1f;
    UPROPERTY()
    int32 NanitePositionPrecision = 0;
    UPROPERTY()
    float NaniteMaxEdgeLengthFactor = 16.0f;
    UPROPERTY()
    bool bDisableRuntimeGrassMapGeneration = false;
    UPROPERTY()
    int32 MaxLODLevel = -1;
    UPROPERTY()
    float LODDistanceFactor = 1.0f;
    UPROPERTY()
    TEnumAsByte<ELandscapeLODFalloff::Type> LODFalloff = ELandscapeLODFalloff::Linear;
    UPROPERTY()
    float LOD0ScreenSize = 0.5f;
    UPROPERTY()
    uint32 LODGroupKey = 0;
    UPROPERTY()
    float LOD0DistributionSetting = 1.25f;
    UPROPERTY()
    float LODDistributionSetting = 3.0f;
    UPROPERTY()
    FPerQualityLevelFloat ScalableLOD0ScreenSize{0.5f};
    UPROPERTY()
    FPerQualityLevelFloat ScalableLOD0DistributionSetting{1.25f};
    UPROPERTY()
    FPerQualityLevelFloat ScalableLODDistributionSetting{3.0f};
    UPROPERTY()
    bool bUseScalableLODSettings = false;
    UPROPERTY()
    float LODBlendRange = 0.1f;
    UPROPERTY()
    float ComponentScreenSizeToUseSubSections = 0.65f;
    UPROPERTY()
    float TessellationComponentScreenSize = 0.8f;
    UPROPERTY()
    bool UseTessellationComponentScreenSizeFalloff = true;
    UPROPERTY()
    float TessellationComponentScreenSizeFalloff = 0.75f;
    UPROPERTY()
    int32 OccluderGeometryLOD = 1;
    UPROPERTY()
    int32 StaticLightingLOD = 0;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> DefaultPhysMaterial = nullptr;
    UPROPERTY()
    float StreamingDistanceMultiplier = 1.0f;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> LandscapeMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> LandscapeHoleMaterial = nullptr;
    UPROPERTY()
    TArray<FLandscapeProxyMaterialOverride> LandscapeMaterialsOverride;
    UPROPERTY()
    bool bMeshHoles = false;
    UPROPERTY()
    uint8 MeshHolesMaxLod = 6;
    UPROPERTY()
    TArray<TObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextures;
    UPROPERTY()
    bool bSetCreateRuntimeVirtualTextureVolumes = false;
    UPROPERTY()
    bool bVirtualTextureRenderWithQuad = false;
    UPROPERTY()
    bool bVirtualTextureRenderWithQuadHQ = true;
    UPROPERTY()
    int32 VirtualTextureNumLods = 6;
    UPROPERTY()
    int32 VirtualTextureLodBias = 0;
    UPROPERTY()
    ERuntimeVirtualTextureMainPassType VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Always;
    UPROPERTY()
    float NegativeZBoundsExtension = 0.0f;
    UPROPERTY()
    float PositiveZBoundsExtension = 0.0f;
    UPROPERTY()
    TArray<TObjectPtr<ULandscapeComponent>> LandscapeComponents;
    UPROPERTY()
    TArray<TObjectPtr<ULandscapeHeightfieldCollisionComponent>> CollisionComponents;
    UPROPERTY()
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> FoliageComponents;
    UPROPERTY()
    TObjectPtr<ULandscapeNaniteComponent> NaniteComponent = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<ULandscapeNaniteComponent>> NaniteComponents;
    UPROPERTY()
    float GrassTypesMaxDiscardDistance = 0.0f;
    UPROPERTY()
    float StaticLightingResolution = 1.0f;
    UPROPERTY()
    uint8 CastShadow : 1 = true;
    UPROPERTY()
    uint8 bCastDynamicShadow : 1 = true;
    UPROPERTY()
    uint8 bCastStaticShadow : 1 = true;
    UPROPERTY()
    EShadowCacheInvalidationBehavior ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Auto;
    UPROPERTY()
    uint8 bCastContactShadow : 1 = true;
    UPROPERTY()
    uint8 bCastFarShadow : 1 = true;
    UPROPERTY()
    uint8 bCastHiddenShadow : 1 = false;
    UPROPERTY()
    uint8 bCastShadowAsTwoSided : 1 = false;
    UPROPERTY()
    uint8 bAffectDistanceFieldLighting : 1 = true;
    UPROPERTY()
    uint8 bAffectDynamicIndirectLighting : 1 = true;
    UPROPERTY()
    uint8 bAffectIndirectLightingWhileHidden : 1 = false;
    UPROPERTY()
    FLightingChannels LightingChannels;
    UPROPERTY()
    uint8 bUseMaterialPositionOffsetInStaticLighting : 1 = false;
    UPROPERTY()
    float NonNaniteVirtualShadowMapConstantDepthBias = 150.0f;
    UPROPERTY()
    float NonNaniteVirtualShadowMapInvalidationHeightErrorThreshold = 250.0f;
    UPROPERTY()
    float NonNaniteVirtualShadowMapInvalidationScreenSizeLimit = 0.2f;
    UPROPERTY()
    uint8 bHoldout : 1 = false;
    UPROPERTY()
    uint8 bRenderCustomDepth : 1 = false;
    UPROPERTY()
    ERendererStencilMask CustomDepthStencilWriteMask = ERendererStencilMask::ERSM_Default;
    UPROPERTY()
    int32 CustomDepthStencilValue = 0;
    UPROPERTY()
    float LDMaxDrawDistance = 0.0f;
    UPROPERTY()
    FLightmassPrimitiveSettings LightmassSettings;
    UPROPERTY()
    TMap<FLandscapeGrassName, TObjectPtr<ULandscapeGrassType>> GrassTypesOverrides;
    UPROPERTY()
    int32 CollisionMipLevel = 0;
    UPROPERTY()
    int32 SimpleCollisionMipLevel = 0;
    UPROPERTY()
    float CollisionThickness = 16.0f;
    UPROPERTY()
    FBodyInstance BodyInstance;
    UPROPERTY()
    uint8 bGenerateOverlapEvents : 1 = false;
    UPROPERTY()
    uint8 bBakeMaterialPositionOffsetIntoCollision : 1 = false;
    UPROPERTY()
    TMap<TObjectPtr<UTexture2D>, TObjectPtr<ULandscapeWeightmapUsage>> WeightmapUsageMap;
    UPROPERTY()
    int32 ComponentSizeQuads = 0;
    UPROPERTY()
    int32 SubsectionSizeQuads = 0;
    UPROPERTY()
    int32 NumSubsections = 0;
    UPROPERTY()
    uint8 bUsedForNavigation : 1 = true;
    UPROPERTY()
    uint8 bFillCollisionUnderLandscapeForNavmesh : 1 = false;
    UPROPERTY()
    ENavDataGatheringMode NavigationGeometryGatheringMode = ENavDataGatheringMode::Default;
    UPROPERTY()
    bool bUseDynamicMaterialInstance = false;
    UPROPERTY()
    bool bUseLandscapeForCullingInvisibleHLODVertices = false;
    UPROPERTY()
    bool bHasLandscapeGrass = true;
    UPROPERTY()
    bool bHasLayersContent = false;
    UPROPERTY()
    bool bUseCompressedHeightmapStorage = false;
    UPROPERTY()
    bool bStripPhysicsWhenCookedClient = false;
    UPROPERTY()
    bool bStripPhysicsWhenCookedServer = false;
    UPROPERTY()
    bool bStripGrassWhenCookedClient = false;
    UPROPERTY()
    bool bStripGrassWhenCookedServer = false;
    UPROPERTY()
    TMap<FName, FLandscapeTargetLayerSettings> TargetLayers;
    UPROPERTY()
    bool bIsProxy = true;
};

}
