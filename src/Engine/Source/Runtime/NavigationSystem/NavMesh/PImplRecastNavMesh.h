#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

using dtPolyRef = uint64;
using dtTileRef = uint64;

constexpr int32 DT_VERTS_PER_POLYGON = 6;

struct dtNavMeshParams {
    float orig[3] = {};
    float tileWidth = 0.0f;
    float tileHeight = 0.0f;
    int32 maxTiles = 0;
    int32 maxPolys = 0;
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
    float walkableHeight = 0.0f;
    float walkableRadius = 0.0f;
    float walkableClimb = 0.0f;
    float bmin[3] = {};
    float bmax[3] = {};
    float bvQuantFactor = 0.0f;
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
    float pos[6] = {};
    float rad = 0.0f;
    uint16 poly = 0;
    uint8 flags = 0;
    uint8 side = 0;
    uint32 userId = 0;
};

struct dtOffMeshSegmentConnection {
    float startA[3] = {};
    float endA[3] = {};
    float startB[3] = {};
    float endB[3] = {};
    float rad = 0.0f;
    uint16 firstPoly = 0;
    uint8 npolys = 0;
    uint8 flags = 0;
    uint32 userId = 0;
};

struct dtCluster {
    float center[3] = {};
    dtPolyRef centerPoly = 0;
    uint32 firstLink = 0;
    uint32 numLinks = 0;
};

//! The tile blob that UE passes to dtNavMesh::addTile, without the links that addTile builds.
struct FRecastMeshTile {
    dtTileRef TileRef = 0;
    dtMeshHeader Header;
    TArray<FVector3f> NavVerts;
    TArray<dtPoly> NavPolys;
    TArray<dtPolyDetail> DetailMeshes;
    TArray<FVector3f> DetailVerts;
    TArray<uint8> DetailTris;
    TArray<dtBVNode> BVTree;
    TArray<dtOffMeshConnection> OffMeshCons;
    TArray<dtOffMeshSegmentConnection> OffMeshSegs;
    TArray<dtCluster> Clusters;
    TArray<uint16> PolyClusters;
    TArray<uint16> ClusterCons;
};

class FPImplRecastNavMesh {
public:
    void Serialize(FArchive& Ar, uint32 NavMeshVersion);

    const dtNavMeshParams& GetParams() const { return Params; }
    const TArray<FRecastMeshTile>& GetTiles() const { return Tiles; }

private:
    void SerializeRecastMeshTile(FArchive& Ar, uint32 NavMeshVersion, FRecastMeshTile& Tile);

    dtNavMeshParams Params;
    TArray<FRecastMeshTile> Tiles;
};

}
