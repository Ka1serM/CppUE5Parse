#pragma once

#include <optional>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavigationDataResolution.h"

namespace ue5parse::runtime {

class ARecastNavMesh;

using dtReal = double;
using dtPolyRef = uint64;
using dtTileRef = uint64;

constexpr int32 DT_VERTS_PER_POLYGON = 6;
constexpr int32 DT_RESOLUTION_COUNT = 3;

struct dtNavMeshResParams {
    dtReal bvQuantFactor = 0.0;
};

struct dtNavMeshParams {
    dtReal walkableHeight = 0.0;
    dtReal walkableRadius = 0.0;
    dtReal walkableClimb = 0.0;
    dtNavMeshResParams resolutionParams[DT_RESOLUTION_COUNT];
    dtReal orig[3] = {};
    dtReal tileWidth = 0.0;
    dtReal tileHeight = 0.0;
    int32 maxTiles = 0;
    int32 maxPolys = 0;
};

struct FDetourTileSizeInfo {
    int32 VertCount = 0;
    int32 PolyCount = 0;
    int32 MaxLinkCount = 0;
    int32 DetailMeshCount = 0;
    int32 DetailVertCount = 0;
    int32 DetailTriCount = 0;
    int32 BvNodeCount = 0;
    int32 OffMeshConCount = 0;
    int32 OffMeshSegConCount = 0;
    int32 ClusterCount = 0;
    int32 OffMeshBase = 0;
};

struct dtMeshHeader {
    int32 magic = 0;
    int32 version = 0;
    int32 x = 0;
    int32 y = 0;
    int32 layer = 0;
    uint32 userId = 0;
    int32 polyCount = 0;
    int32 vertCount = 0;
    int32 maxLinkCount = 0;
    int32 detailMeshCount = 0;
    int32 detailVertCount = 0;
    int32 detailTriCount = 0;
    int32 bvNodeCount = 0;
    int32 offMeshConCount = 0;
    int32 offMeshSegConCount = 0;
    int32 offMeshBase = 0;
    int32 offMeshSegPolyBase = 0;
    int32 offMeshSegVertBase = 0;
    uint8 resolution = 0;
    dtReal walkableHeight = 0.0;
    dtReal walkableRadius = 0.0;
    dtReal walkableClimb = 0.0;
    dtReal bmin[3] = {};
    dtReal bmax[3] = {};
    dtReal bvQuantFactor = 0.0;
    int32 clusterCount = 0;
    int32 maxClusterCons = 0;
};

struct dtPoly {
    uint32 firstLink = 0;
    uint16 verts[DT_VERTS_PER_POLYGON] = {};
    uint16 neis[DT_VERTS_PER_POLYGON] = {};
    uint16 flags = 0;
    uint8 vertCount = 0;
    uint8 areaAndtype = 0;
};

struct dtPolyDetail {
    uint32 vertBase = 0;
    uint32 triBase = 0;
    uint8 vertCount = 0;
    uint8 triCount = 0;
};

struct dtBVNode {
    uint16 bmin[3] = {};
    uint16 bmax[3] = {};
    int32 i = 0;
};

struct dtOffMeshConnection {
    dtReal pos[6] = {};
    dtReal rad = 0.0;
    dtReal height = 0.0;
    uint64 userId = 0;
    uint16 poly = 0;
    uint8 side = 0;
    uint8 flags = 0;
};

struct dtOffMeshSegmentConnection {
    dtReal startA[3] = {};
    dtReal endA[3] = {};
    dtReal startB[3] = {};
    dtReal endB[3] = {};
    dtReal rad = 0.0;
    uint32 userId = 0;
    uint16 firstPoly = 0;
    uint8 npolys = 0;
    uint8 flags = 0;
};

struct dtCluster {
    dtReal center[3] = {};
    dtPolyRef centerPoly = 0;
    uint32 firstLink = 0;
    uint32 numLinks = 0;
};

struct dtTileCacheLayerHeader {
    int16 version = 0;
    int32 tx = 0;
    int32 ty = 0;
    int32 tlayer = 0;
    uint16 hmin = 0;
    uint16 hmax = 0;
    uint16 width = 0;
    uint16 height = 0;
    uint16 minx = 0;
    uint16 maxx = 0;
    uint16 miny = 0;
    uint16 maxy = 0;
    dtReal bmin[3] = {};
    dtReal bmax[3] = {};
};

//! The tile blob that UE passes to dtNavMesh::addTile, without the links that addTile builds, and its compressed tile cache layer.
struct FRecastMeshTile {
    dtTileRef TileRef = 0;
    dtMeshHeader Header;
    //! World space, also for NAVMESHVER_TILEVERT_ENCODING, which stores them relative to Header.bmin.
    TArray<FVector> NavVerts;
    TArray<dtPoly> NavPolys;
    TArray<dtPolyDetail> DetailMeshes;
    //! World space, also for NAVMESHVER_TILEVERT_ENCODING, which stores them relative to Header.bmin.
    TArray<FVector> DetailVerts;
    TArray<uint8> DetailTris;
    TArray<dtBVNode> BVTree;
    TArray<dtOffMeshConnection> OffMeshCons;
    TArray<dtOffMeshSegmentConnection> OffMeshSegs;
    TArray<dtCluster> Clusters;
    TArray<uint16> PolyClusters;
    TArray<uint16> ClusterCons;
    std::optional<dtTileCacheLayerHeader> TileCacheLayerHeader;
    TArray<uint8> TileCacheCompressedData;
};

class FPImplRecastNavMesh {
public:
    explicit FPImplRecastNavMesh(const ARecastNavMesh* Owner) : NavMeshOwner(Owner) {}

    void Serialize(FArchive& Ar, uint32 NavMeshVersion);

    static void SerializeRecastMeshTile(FArchive& Ar, uint32 NavMeshVersion, FRecastMeshTile& Tile);
    static void SerializeCompressedTileCacheData(FArchive& Ar, FRecastMeshTile& Tile);

    const dtNavMeshParams& GetParams() const { return Params; }
    const TArray<FRecastMeshTile>& GetTiles() const { return Tiles; }

private:
    const ARecastNavMesh* NavMeshOwner = nullptr;
    dtNavMeshParams Params;
    TArray<FRecastMeshTile> Tiles;
};

}
