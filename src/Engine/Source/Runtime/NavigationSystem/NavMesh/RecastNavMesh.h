#pragma once

#include <memory>

#include "Engine/Source/Runtime/NavigationSystem/NavigationData.h"
#include "Engine/Source/Runtime/NavigationSystem/NavMesh/PImplRecastNavMesh.h"
#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastVersion.h"

#include "RecastNavMesh.generated.h"

namespace ue5parse::runtime {

namespace ERecastPartitioning {

UENUM()
enum Type : int32 { Monotone, Watershed, ChunkyMonotone };

}

UCLASS()
class ARecastNavMesh : public ANavigationData {
    GENERATED_BODY()
public:
    explicit ARecastNavMesh(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    uint32 GetNavMeshVersion() const { return NavMeshVersion; }
    const FPImplRecastNavMesh* GetRecastNavMeshImpl() const { return RecastNavMeshImpl.get(); }

    UPROPERTY()
    uint32 bDrawTriangleEdges : 1 = 0;
    UPROPERTY()
    uint32 bDrawPolyEdges : 1 = 0;
    UPROPERTY()
    uint32 bDrawNavMeshEdges : 1 = 1;
    UPROPERTY()
    uint32 bDrawTileBounds : 1 = 0;
    UPROPERTY()
    uint32 bDrawPathCollidingGeometry : 1 = 0;
    UPROPERTY()
    uint32 bDrawTileLabels : 1 = 0;
    UPROPERTY()
    uint32 bDrawPolygonLabels : 1 = 0;
    UPROPERTY()
    uint32 bDrawDefaultPolygonCost : 1 = 0;
    UPROPERTY()
    uint32 bDrawLabelsOnPathNodes : 1 = 0;
    UPROPERTY()
    uint32 bDrawNavLinks : 1 = 1;
    UPROPERTY()
    uint32 bDrawFailedNavLinks : 1 = 0;
    UPROPERTY()
    uint32 bDrawClusters : 1 = 0;
    UPROPERTY()
    uint32 bDistinctlyDrawTilesBeingBuilt : 1 = 1;
    UPROPERTY()
    float DrawOffset = 10.0f;
    UPROPERTY()
    float TileSizeUU = 1000.0f;
    UPROPERTY()
    float CellSize = 19.0f;
    UPROPERTY()
    float CellHeight = 10.0f;
    UPROPERTY()
    float AgentRadius = 34.0f;
    UPROPERTY()
    float AgentHeight = 144.0f;
    UPROPERTY()
    float AgentMaxHeight = 160.0f;
    UPROPERTY()
    float AgentMaxSlope = 44.0f;
    UPROPERTY()
    float AgentMaxStepHeight = 35.0f;
    UPROPERTY()
    float MinRegionArea = 0.0f;
    UPROPERTY()
    float MergeRegionSize = 400.0f;
    UPROPERTY()
    float MaxSimplificationError = 1.3f;
    UPROPERTY()
    float DefaultDrawDistance = 5000.0f;
    UPROPERTY()
    float DefaultMaxSearchNodes = 2048.0f;
    UPROPERTY()
    float DefaultMaxHierarchicalSearchNodes = 2048.0f;
    UPROPERTY()
    TEnumAsByte<ERecastPartitioning::Type> RegionPartitioning = ERecastPartitioning::Watershed;
    UPROPERTY()
    TEnumAsByte<ERecastPartitioning::Type> LayerPartitioning = ERecastPartitioning::Watershed;
    UPROPERTY()
    int32 RegionChunkSplits = 2;
    UPROPERTY()
    int32 LayerChunkSplits = 2;
    UPROPERTY()
    int32 MaxTileGridWidth = 256;
    UPROPERTY()
    int32 MaxTileGridHeight = 256;
    UPROPERTY()
    uint32 bSortNavigationAreasByCost : 1 = 0;
    UPROPERTY()
    uint32 bPerformVoxelFiltering : 1 = 1;
    UPROPERTY()
    uint32 bUseBetterOffsetsFromCorners : 1 = 1;
    UPROPERTY()
    uint32 bUseVoxelCache : 1 = 0;
    UPROPERTY()
    float TileSetUpdateInterval = 1.0f;
    UPROPERTY()
    float HeuristicScale = 0.999f;

private:
    uint32 NavMeshVersion = 0;
    std::unique_ptr<FPImplRecastNavMesh> RecastNavMeshImpl;
};

}
