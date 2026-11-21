#include "Engine/Source/Runtime/NavigationSystem/NavMesh/PImplRecastNavMesh.h"

#include <limits>

#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMesh.h"

namespace ue5parse::runtime {

namespace {

template <class T, class FSerializeElement>
void SerializeElements(FArchive& Ar, int32 Count, TArray<T>& Array, FSerializeElement SerializeElement) {
    Array.SetNum(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        SerializeElement(Array[Index]);
}

void SerializeHeader(FArchive& Ar, uint32 NavMeshVersion, dtMeshHeader& H) {
    Ar << H.magic << H.version << H.x << H.y;
    Ar << H.layer << H.userId << H.polyCount << H.vertCount;
    Ar << H.maxLinkCount << H.detailMeshCount << H.detailVertCount << H.detailTriCount;
    Ar << H.bvNodeCount << H.offMeshConCount << H.offMeshBase;
    Ar << H.walkableHeight << H.walkableRadius << H.walkableClimb;
    Ar << H.bmin[0] << H.bmin[1] << H.bmin[2];
    Ar << H.bmax[0] << H.bmax[1] << H.bmax[2];
    Ar << H.bvQuantFactor;
    Ar << H.clusterCount;
    if (NavMeshVersion < NAVMESHVER_CLUSTER_SIMPLIFIED)
        Ar << H.maxClusterCons;
    Ar << H.offMeshSegConCount << H.offMeshSegPolyBase << H.offMeshSegVertBase;
}

}

void FPImplRecastNavMesh::Serialize(FArchive& Ar, uint32 NavMeshVersion) {
    const int32 NumTiles = Ar.ReadCount(12);
    Ar << Params.orig[0] << Params.orig[1] << Params.orig[2];
    Ar << Params.tileWidth << Params.tileHeight << Params.maxTiles << Params.maxPolys;
    for (int32 Index = 0; Index < NumTiles && !Ar.IsError(); Index++) {
        dtTileRef TileRef = std::numeric_limits<dtTileRef>::max();
        int32 TileDataSize = 0;
        Ar << TileRef << TileDataSize;
        if (TileRef == std::numeric_limits<dtTileRef>::max() || TileDataSize == 0)
            continue;
        FRecastMeshTile& Tile = Tiles[Tiles.AddDefaulted()];
        Tile.TileRef = TileRef;
        SerializeRecastMeshTile(Ar, NavMeshVersion, Tile);
    }
}

void FPImplRecastNavMesh::SerializeRecastMeshTile(FArchive& Ar, uint32 NavMeshVersion, FRecastMeshTile& Tile) {
    const int32 totVertCount = Ar.ReadCount(12);
    const int32 totPolyCount = Ar.ReadCount(32);
    const int32 maxLinkCount = Ar.ReadCount(0);
    const int32 detailMeshCount = Ar.ReadCount(10);
    const int32 detailVertCount = Ar.ReadCount(12);
    const int32 detailTriCount = Ar.ReadCount(4);
    const int32 bvNodeCount = Ar.ReadCount(16);
    const int32 offMeshConCount = Ar.ReadCount(36);
    const int32 offMeshSegConCount = Ar.ReadCount(60);
    const int32 clusterCount = Ar.ReadCount(12);
    const int32 clusterConCount = NavMeshVersion < NAVMESHVER_CLUSTER_SIMPLIFIED ? Ar.ReadCount(2) : 0;
    const int32 polyClusterCount = detailMeshCount;
    static_cast<void>(maxLinkCount);
    if (Ar.IsError())
        return;
    SerializeHeader(Ar, NavMeshVersion, Tile.Header);
    SerializeElements(Ar, totVertCount, Tile.NavVerts, [&](FVector3f& V) { Ar << V.X << V.Y << V.Z; });
    SerializeElements(Ar, totPolyCount, Tile.NavPolys, [&](dtPoly& P) {
        Ar << P.firstLink;
        for (uint16& Vert : P.verts)
            Ar << Vert;
        for (uint16& Nei : P.neis)
            Ar << Nei;
        Ar << P.flags << P.vertCount << P.areaAndtype;
    });
    SerializeElements(Ar, detailMeshCount, Tile.DetailMeshes, [&](dtPolyDetail& DM) { Ar << DM.vertBase << DM.triBase << DM.vertCount << DM.triCount; });
    SerializeElements(Ar, detailVertCount, Tile.DetailVerts, [&](FVector3f& V) { Ar << V.X << V.Y << V.Z; });
    SerializeElements(Ar, detailTriCount * 4, Tile.DetailTris, [&](uint8& V) { Ar << V; });
    SerializeElements(Ar, bvNodeCount, Tile.BVTree, [&](dtBVNode& Node) {
        Ar << Node.bmin[0] << Node.bmin[1] << Node.bmin[2];
        Ar << Node.bmax[0] << Node.bmax[1] << Node.bmax[2];
        Ar << Node.i;
    });
    SerializeElements(Ar, offMeshConCount, Tile.OffMeshCons, [&](dtOffMeshConnection& Conn) {
        for (float& Pos : Conn.pos)
            Ar << Pos;
        Ar << Conn.rad << Conn.poly << Conn.flags << Conn.side << Conn.userId;
    });
    SerializeElements(Ar, offMeshSegConCount, Tile.OffMeshSegs, [&](dtOffMeshSegmentConnection& Seg) {
        Ar << Seg.startA[0] << Seg.startA[1] << Seg.startA[2];
        Ar << Seg.startB[0] << Seg.startB[1] << Seg.startB[2];
        Ar << Seg.endA[0] << Seg.endA[1] << Seg.endA[2];
        Ar << Seg.endB[0] << Seg.endB[1] << Seg.endB[2];
        Ar << Seg.rad << Seg.firstPoly << Seg.npolys << Seg.flags << Seg.userId;
    });
    SerializeElements(Ar, clusterCount, Tile.Clusters, [&](dtCluster& Cluster) {
        if (NavMeshVersion < NAVMESHVER_CLUSTER_SIMPLIFIED)
            Ar << Cluster.centerPoly << Cluster.firstLink << Cluster.numLinks;
        Ar << Cluster.center[0] << Cluster.center[1] << Cluster.center[2];
    });
    SerializeElements(Ar, polyClusterCount, Tile.PolyClusters, [&](uint16& C) { Ar << C; });
    SerializeElements(Ar, clusterConCount, Tile.ClusterCons, [&](uint16& C) { Ar << C; });
}

}
