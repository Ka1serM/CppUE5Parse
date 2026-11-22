#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavRelevantInterface.h"
#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodyInstance.h"
#include "Engine/Source/Runtime/Engine/SceneTypes.h"

#include "FoliageType.generated.h"

namespace ue5parse::runtime {

class URuntimeVirtualTexture;

UENUM()
enum FoliageVertexColorMask : int32 {
    FOLIAGEVERTEXCOLORMASK_Disabled,
    FOLIAGEVERTEXCOLORMASK_Red,
    FOLIAGEVERTEXCOLORMASK_Green,
    FOLIAGEVERTEXCOLORMASK_Blue,
    FOLIAGEVERTEXCOLORMASK_Alpha,
};

UENUM()
enum class EVertexColorMaskChannel : uint8 {
    Red,
    Green,
    Blue,
    Alpha,
    MAX_None,
};

USTRUCT()
struct FFoliageVertexColorChannelMask {
    GENERATED_BODY()
    UPROPERTY()
    uint32 UseMask : 1 = false;
    UPROPERTY()
    float MaskThreshold = 0.5f;
    UPROPERTY()
    uint32 InvertMask : 1 = false;
};

UENUM()
enum class EFoliageScaling : uint8 {
    Uniform,
    Free,
    LockXY,
    LockXZ,
    LockYZ,
};

USTRUCT()
struct FFoliageDensityFalloff {
    GENERATED_BODY()
    UPROPERTY()
    bool bUseFalloffCurve = false;
    UPROPERTY()
    FRuntimeFloatCurve FalloffCurve;
};

UCLASS(Abstract)
class UFoliageType : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    FGuid UpdateGuid;
    UPROPERTY()
    float Density = 100.0f;
    UPROPERTY()
    float DensityAdjustmentFactor = 1.0f;
    UPROPERTY()
    float Radius = 0.0f;
    UPROPERTY()
    bool bSingleInstanceModeOverrideRadius = false;
    UPROPERTY()
    float SingleInstanceModeRadius = 0.0f;
    UPROPERTY()
    EFoliageScaling Scaling = EFoliageScaling::Uniform;
    UPROPERTY()
    FFloatInterval ScaleX{.Min = 1.0f, .Max = 1.0f};
    UPROPERTY()
    FFloatInterval ScaleY{.Min = 1.0f, .Max = 1.0f};
    UPROPERTY()
    FFloatInterval ScaleZ{.Min = 1.0f, .Max = 1.0f};
    UPROPERTY()
    FFoliageVertexColorChannelMask VertexColorMaskByChannel[static_cast<uint8>(EVertexColorMaskChannel::MAX_None)];
    UPROPERTY()
    TEnumAsByte<FoliageVertexColorMask> VertexColorMask_DEPRECATED = FOLIAGEVERTEXCOLORMASK_Disabled;
    UPROPERTY()
    float VertexColorMaskThreshold_DEPRECATED = 0.0f;
    UPROPERTY()
    uint32 VertexColorMaskInvert_DEPRECATED : 1 = false;
    UPROPERTY()
    FFloatInterval ZOffset;
    UPROPERTY()
    uint32 AlignToNormal : 1 = true;
    UPROPERTY()
    uint32 AverageNormal : 1 = false;
    UPROPERTY()
    uint32 AverageNormalSingleComponent : 1 = true;
    UPROPERTY()
    float AlignMaxAngle = 0.0f;
    UPROPERTY()
    uint32 RandomYaw : 1 = true;
    UPROPERTY()
    float RandomPitchAngle = 0.0f;
    UPROPERTY()
    FFloatInterval GroundSlopeAngle{.Min = 0.0f, .Max = 45.0f};
    UPROPERTY()
    FFloatInterval Height{.Min = -262144.0f, .Max = 262144.0f};
    UPROPERTY()
    TArray<FName> LandscapeLayers;
    UPROPERTY()
    float MinimumLayerWeight = 0.5f;
    UPROPERTY()
    TArray<FName> ExclusionLandscapeLayers;
    UPROPERTY()
    float MinimumExclusionLayerWeight = 0.0f;
    UPROPERTY()
    FName LandscapeLayer_DEPRECATED;
    UPROPERTY()
    uint32 CollisionWithWorld : 1 = false;
    UPROPERTY()
    FVector CollisionScale{0.9, 0.9, 0.9};
    UPROPERTY()
    int32 AverageNormalSampleCount = 10;
    UPROPERTY()
    FBoxSphereBounds MeshBounds;
    UPROPERTY()
    FVector LowBoundOriginRadius;
    UPROPERTY()
    TEnumAsByte<EComponentMobility::Type> Mobility = EComponentMobility::Static;
    UPROPERTY()
    FInt32Interval CullDistance;
    UPROPERTY()
    uint32 bEnableStaticLighting_DEPRECATED : 1 = true;
    UPROPERTY()
    uint32 CastShadow : 1 = true;
    UPROPERTY()
    uint32 bAffectDynamicIndirectLighting : 1 = true;
    UPROPERTY()
    uint32 bAffectDistanceFieldLighting : 1 = false;
    UPROPERTY()
    uint32 bCastDynamicShadow : 1 = true;
    UPROPERTY()
    uint32 bCastStaticShadow : 1 = true;
    UPROPERTY()
    uint8 bCastContactShadow : 1 = true;
    UPROPERTY()
    uint32 bCastShadowAsTwoSided : 1 = false;
    UPROPERTY()
    uint32 bCastHiddenShadow : 1 = false;
    UPROPERTY()
    uint32 bReceivesDecals : 1 = false;
    UPROPERTY()
    uint32 bOverrideLightMapRes : 1 = false;
    UPROPERTY()
    int32 OverriddenLightMapRes = 8;
    UPROPERTY()
    ELightmapType LightmapType = ELightmapType::Default;
    UPROPERTY()
    uint32 bUseAsOccluder : 1 = false;
    UPROPERTY()
    uint8 bVisibleInRayTracing : 1 = true;
    UPROPERTY()
    uint8 bVisibleInReflections : 1 = true;
    UPROPERTY()
    uint8 bEvaluateWorldPositionOffset : 1 = true;
    UPROPERTY()
    int32 WorldPositionOffsetDisableDistance = 0;
    UPROPERTY()
    float NanitePixelProgrammableDistance = 0.0f;
    UPROPERTY()
    EShadowCacheInvalidationBehavior ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Auto;
    UPROPERTY()
    FBodyInstance BodyInstance;
    UPROPERTY()
    TEnumAsByte<EHasCustomNavigableGeometry::Type> CustomNavigableGeometry = EHasCustomNavigableGeometry::No;
    UPROPERTY()
    FLightingChannels LightingChannels;
    UPROPERTY()
    uint32 bRenderCustomDepth : 1 = false;
    UPROPERTY()
    ERendererStencilMask CustomDepthStencilWriteMask = ERendererStencilMask::ERSM_Default;
    UPROPERTY()
    int32 CustomDepthStencilValue = 0;
    UPROPERTY()
    int32 TranslucencySortPriority = 0;
    UPROPERTY()
    float CollisionRadius = 100.0f;
    UPROPERTY()
    float ShadeRadius = 100.0f;
    UPROPERTY()
    int32 NumSteps = 3;
    UPROPERTY()
    float InitialSeedDensity = 1.0f;
    UPROPERTY()
    float AverageSpreadDistance = 50.0f;
    UPROPERTY()
    float SpreadVariance = 150.0f;
    UPROPERTY()
    int32 SeedsPerStep = 3;
    UPROPERTY()
    int32 DistributionSeed = 0;
    UPROPERTY()
    float MaxInitialSeedOffset = 0.0f;
    UPROPERTY()
    bool bCanGrowInShade = false;
    UPROPERTY()
    bool bSpawnsInShade = false;
    UPROPERTY()
    float MaxInitialAge = 0.0f;
    UPROPERTY()
    float MaxAge = 10.0f;
    UPROPERTY()
    float OverlapPriority = 0.0f;
    UPROPERTY()
    FFloatInterval ProceduralScale{.Min = 1.0f, .Max = 3.0f};
    UPROPERTY()
    FRuntimeFloatCurve ScaleCurve;
    UPROPERTY()
    FFoliageDensityFalloff DensityFalloff;
    UPROPERTY()
    int32 ChangeCount = 0;
    UPROPERTY()
    uint32 ReapplyDensity : 1 = false;
    UPROPERTY()
    uint32 ReapplyRadius : 1 = false;
    UPROPERTY()
    uint32 ReapplyAlignToNormal : 1 = false;
    UPROPERTY()
    uint32 ReapplyRandomYaw : 1 = false;
    UPROPERTY()
    uint32 ReapplyScaling : 1 = false;
    UPROPERTY()
    uint32 ReapplyScaleX : 1 = false;
    UPROPERTY()
    uint32 ReapplyScaleY : 1 = false;
    UPROPERTY()
    uint32 ReapplyScaleZ : 1 = false;
    UPROPERTY()
    uint32 ReapplyRandomPitchAngle : 1 = false;
    UPROPERTY()
    uint32 ReapplyGroundSlope : 1 = false;
    UPROPERTY()
    uint32 ReapplyHeight : 1 = false;
    UPROPERTY()
    uint32 ReapplyLandscapeLayers : 1 = false;
    UPROPERTY()
    uint32 ReapplyLandscapeLayer : 1 = false;
    UPROPERTY()
    uint32 ReapplyZOffset : 1 = false;
    UPROPERTY()
    uint32 ReapplyCollisionWithWorld : 1 = false;
    UPROPERTY()
    uint32 ReapplyVertexColorMask : 1 = false;
    UPROPERTY()
    uint32 bEnableDensityScaling : 1 = false;
    UPROPERTY()
    uint32 bEnableDiscardOnLoad : 1 = false;
    UPROPERTY()
    uint32 bEnableCullDistanceScaling : 1 = false;
    UPROPERTY()
    TArray<TObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextures;
    UPROPERTY()
    int32 VirtualTextureCullMips = 0;
    UPROPERTY()
    ERuntimeVirtualTextureMainPassType VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;
    UPROPERTY()
    uint32 IsSelected : 1 = false;
    UPROPERTY()
    uint64 HiddenEditorViews = 0;
    UPROPERTY()
    int32 DisplayOrder = 0;
    UPROPERTY()
    float MaxClusterRadius = 0.0f;
    UPROPERTY()
    int32 MaxInstancesPerCluster = 0;
    UPROPERTY()
    float ScaleMinX = 1.0f;
    UPROPERTY()
    float ScaleMinY = 1.0f;
    UPROPERTY()
    float ScaleMinZ = 1.0f;
    UPROPERTY()
    float ScaleMaxX = 1.0f;
    UPROPERTY()
    float ScaleMaxY = 1.0f;
    UPROPERTY()
    float ScaleMaxZ = 1.0f;
    UPROPERTY()
    uint32 LockScaleX : 1 = false;
    UPROPERTY()
    uint32 LockScaleY : 1 = false;
    UPROPERTY()
    uint32 LockScaleZ : 1 = false;
    UPROPERTY()
    uint32 UniformScale : 1 = true;
    UPROPERTY()
    float GroundSlope = 45.0f;
    UPROPERTY()
    float HeightMin = -262144.0f;
    UPROPERTY()
    float HeightMax = 262144.0f;
    UPROPERTY()
    float ZOffsetMin = 0.0f;
    UPROPERTY()
    float ZOffsetMax = 0.0f;
    UPROPERTY()
    float ReapplyDensityAmount = 1.0f;
    UPROPERTY()
    int32 StartCullDistance = 0;
    UPROPERTY()
    int32 EndCullDistance = 0;
    UPROPERTY()
    uint32 ShowNothing : 1 = false;
    UPROPERTY()
    uint32 ShowPaintSettings : 1 = true;
    UPROPERTY()
    uint32 ShowInstanceSettings : 1 = false;
};

}
