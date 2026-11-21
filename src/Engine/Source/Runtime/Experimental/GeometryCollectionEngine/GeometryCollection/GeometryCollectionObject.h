#pragma once

#include <memory>

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Experimental/Chaos/GeometryCollection/ManagedArrayCollection.h"
#include "Engine/Source/Runtime/Experimental/ChaosSolverEngine/Chaos/ChaosSolverActor.h"
#include "Engine/Source/Runtime/Experimental/Dataflow/Engine/Dataflow/DataflowInstance.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionRenderData.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionSimulationTypes.h"

#include "GeometryCollectionObject.generated.h"

namespace ue5parse::runtime {

class UDataflow;
class UMaterialInterface;
class UPhysicalMaterial;
class UStaticMesh;

USTRUCT()
struct FGeometryCollectionSource {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath SourceGeometryObject;
    UPROPERTY()
    FTransform LocalTransform;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> SourceMaterial;
    UPROPERTY()
    bool bAddInternalMaterials = true;
    UPROPERTY()
    bool bSplitComponents = false;
    UPROPERTY()
    TArray<float> InstanceCustomData;
    UPROPERTY()
    bool bSetInternalFromMaterialIndex = false;
};

USTRUCT()
struct FGeometryCollectionEmbeddedExemplar {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath StaticMeshExemplar;
    UPROPERTY()
    float StartCullDistance = 0.0f;
    UPROPERTY()
    float EndCullDistance = 0.0f;
    UPROPERTY()
    int32 InstanceCount = 0;
};

USTRUCT()
struct FGeometryCollectionAutoInstanceMesh {
    GENERATED_BODY()
    UPROPERTY()
    FSoftObjectPath StaticMesh;
    UPROPERTY()
    TObjectPtr<const UStaticMesh> Mesh = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY()
    int32 NumInstances = 0;
    UPROPERTY()
    TArray<float> CustomData;
};

USTRUCT()
struct FGeometryCollectionProxyMeshMaterials {
    GENERATED_BODY()
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> Materials;
};

USTRUCT()
struct FGeometryCollectionProxyMeshData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<TObjectPtr<UStaticMesh>> ProxyMeshes;
    UPROPERTY()
    TArray<FTransform3f> MeshTransforms;
    UPROPERTY()
    TArray<FGeometryCollectionProxyMeshMaterials> MeshOverrideMaterials;
};

USTRUCT()
struct FGeometryCollectionLevelSetData {
    GENERATED_BODY()
    UPROPERTY()
    int32 MinLevelSetResolution = 5;
    UPROPERTY()
    int32 MaxLevelSetResolution = 10;
    UPROPERTY()
    int32 MinClusterLevelSetResolution = 25;
    UPROPERTY()
    int32 MaxClusterLevelSetResolution = 50;
};

USTRUCT()
struct FGeometryCollectionCollisionParticleData {
    GENERATED_BODY()
    UPROPERTY()
    float CollisionParticlesFraction = 1.0f;
    UPROPERTY()
    int32 MaximumCollisionParticles = 60;
};

USTRUCT()
struct FGeometryCollectionCollisionTypeData {
    GENERATED_BODY()
    UPROPERTY()
    ECollisionTypeEnum CollisionType = ECollisionTypeEnum::Chaos_Volumetric;
    UPROPERTY()
    EImplicitTypeEnum ImplicitType = EImplicitTypeEnum::Chaos_Implicit_Convex;
    UPROPERTY()
    FGeometryCollectionLevelSetData LevelSet;
    UPROPERTY()
    FGeometryCollectionCollisionParticleData CollisionParticles;
    UPROPERTY()
    float CollisionObjectReductionPercentage = 0.0f;
    UPROPERTY()
    float CollisionMarginFraction = 0.0f;
};

USTRUCT()
struct FGeometryCollectionSizeSpecificData {
    GENERATED_BODY()
    UPROPERTY()
    float MaxSize = 99999.9f;
    UPROPERTY()
    ECollisionTypeEnum CollisionType = ECollisionTypeEnum::Chaos_Volumetric;
    UPROPERTY()
    EImplicitTypeEnum ImplicitType = EImplicitTypeEnum::Chaos_Implicit_Box;
    UPROPERTY()
    int32 MinLevelSetResolution = 5;
    UPROPERTY()
    int32 MaxLevelSetResolution = 10;
    UPROPERTY()
    int32 MinClusterLevelSetResolution = 25;
    UPROPERTY()
    int32 MaxClusterLevelSetResolution = 50;
    UPROPERTY()
    int32 CollisionObjectReductionPercentage = 0;
    UPROPERTY()
    float CollisionParticlesFraction = 1.0f;
    UPROPERTY()
    int32 MaximumCollisionParticles = 60;
    UPROPERTY()
    TArray<FGeometryCollectionCollisionTypeData> CollisionShapes = {FGeometryCollectionCollisionTypeData()};
    UPROPERTY()
    int32 DamageThreshold = 5000;
};

USTRUCT()
struct FGeometryCollectionDamagePropagationData {
    GENERATED_BODY()
    UPROPERTY()
    bool bEnabled = true;
    UPROPERTY()
    float BreakDamagePropagationFactor = 1.0f;
    UPROPERTY()
    float ShockDamagePropagationFactor = 0.0f;
};

UCLASS()
class UGeometryCollection : public UObject {
    GENERATED_BODY()
public:
    explicit UGeometryCollection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY()
    FGuid PersistentGuid;
    UPROPERTY()
    FGuid StateGuid;
    UPROPERTY()
    int32 InteriorMaterialIndex = INDEX_NONE;
    UPROPERTY()
    int32 BoneSelectedMaterialIndex = INDEX_NONE;
    UPROPERTY()
    ECollisionTypeEnum CollisionType = ECollisionTypeEnum::Chaos_Volumetric;
    UPROPERTY()
    EImplicitTypeEnum ImplicitType = EImplicitTypeEnum::Chaos_Implicit_Box;
    UPROPERTY()
    int32 MinLevelSetResolution = 10;
    UPROPERTY()
    int32 MaxLevelSetResolution = 10;
    UPROPERTY()
    int32 MinClusterLevelSetResolution = 50;
    UPROPERTY()
    int32 MaxClusterLevelSetResolution = 50;
    UPROPERTY()
    float CollisionObjectReductionPercentage = 0.0f;
    UPROPERTY()
    bool bMassAsDensity = true;
    UPROPERTY()
    float Mass = 2500.0f;
    UPROPERTY()
    float MinimumMassClamp = 0.1f;
    UPROPERTY()
    float CollisionParticlesFraction = 1.0f;
    UPROPERTY()
    int32 MaximumCollisionParticles = 60;
    UPROPERTY()
    TArray<FGeometryCollectionSizeSpecificData> SizeSpecificData = {FGeometryCollectionSizeSpecificData()};
    UPROPERTY()
    bool EnableRemovePiecesOnFracture_DEPRECATED = false;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> RemoveOnFractureMaterials_DEPRECATED;
    UPROPERTY()
    TArray<FGeometryCollectionSource> GeometrySource;
    UPROPERTY()
    bool EnableClustering = true;
    UPROPERTY()
    int32 ClusterGroupIndex = 0;
    UPROPERTY()
    int32 MaxClusterLevel = 100;
    UPROPERTY()
    TArray<float> DamageThreshold = {500000.0f, 50000.0f, 5000.0f};
    UPROPERTY()
    EClusterConnectionTypeEnum ClusterConnectionType = EClusterConnectionTypeEnum::Chaos_MinimalSpanningSubsetDelaunayTriangulation;
    UPROPERTY()
    TArray<FGeometryCollectionEmbeddedExemplar> EmbeddedGeometryExemplar;
    UPROPERTY()
    bool bUseFullPrecisionUVs = false;
    UPROPERTY()
    bool bStripOnCook = false;
    UPROPERTY()
    bool EnableNanite = false;
    UPROPERTY()
    bool bRemoveOnMaxSleep = false;
    UPROPERTY()
    FVector2D MaximumSleepTime = FVector2D(5.0, 10.0);
    UPROPERTY()
    FVector2D RemovalDuration = FVector2D(2.5, 5.0);
    UPROPERTY()
    bool bUseSizeSpecificDamageThreshold = false;
    UPROPERTY()
    bool PerClusterOnlyDamageThreshold = false;
    UPROPERTY()
    FGeometryCollectionDamagePropagationData DamagePropagationData;
    UPROPERTY()
    float ConnectionGraphBoundsFilteringMargin = 0.0f;
    UPROPERTY()
    TArray<FGeometryCollectionAutoInstanceMesh> AutoInstanceMeshes;
    UPROPERTY()
    FSoftObjectPath RootProxy;
    UPROPERTY()
    bool bImportCollisionFromSource = false;
    UPROPERTY()
    bool bSlowMovingAsSleeping = true;
    UPROPERTY()
    float SlowMovingVelocityThreshold = 1.0f;
    UPROPERTY()
    TObjectPtr<UDataflow> Dataflow = nullptr;
    UPROPERTY()
    TObjectPtr<UDataflow> DataflowAsset = nullptr;
    UPROPERTY()
    FString DataflowTerminal = "GeometryCollectionTerminal";
    UPROPERTY()
    EDamageModelTypeEnum DamageModel = EDamageModelTypeEnum::Chaos_Damage_Model_UserDefined_Damage_Threshold;
    UPROPERTY()
    bool bStripRenderDataOnCook = false;
    UPROPERTY()
    TObjectPtr<UClass> CustomRendererType = nullptr;
    UPROPERTY()
    FGeometryCollectionProxyMeshData RootProxyData;
    UPROPERTY()
    bool bConvertVertexColorsToSRGB = true;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysicsMaterial = nullptr;
    UPROPERTY()
    bool bDensityFromPhysicsMaterial = false;
    UPROPERTY()
    bool bScaleOnRemoval = true;
    UPROPERTY()
    TMap<FString, FString> Overrides;
    UPROPERTY()
    int32 RootIndex = INDEX_NONE;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> BoneSelectedMaterial = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
    UPROPERTY()
    bool bUseMaterialDamageModifiers = false;
    UPROPERTY()
    float CachedDensityFromPhysicsMaterialInGCm3 = 0.0f;
    UPROPERTY()
    bool bOptimizeConvexes = false;
    UPROPERTY()
    TArray<int32> BreadthFirstTransformIndices;
    UPROPERTY()
    TArray<int32> AutoInstanceTransformRemapIndices;
    UPROPERTY()
    bool bEnableNaniteFallback = false;
    UPROPERTY()
    bool bAutomaticCrumblePartialClusters = true;
    UPROPERTY()
    FDataflowInstance DataflowInstance;
    UPROPERTY()
    uint32 NaniteMinimumResidencyInKB = 0;
    UPROPERTY()
    uint8 bSupportRayTracing : 1 = 1;

    FManagedArrayCollection GeometryCollection;
    std::unique_ptr<FGeometryCollectionRenderData> RenderData;
};

}
