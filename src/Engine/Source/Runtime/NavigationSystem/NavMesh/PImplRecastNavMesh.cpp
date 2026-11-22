#include "Engine/Source/Runtime/NavigationSystem/NavMesh/PImplRecastNavMesh.h"

#include <limits>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMesh.h"

namespace ue5parse::runtime {

namespace {

//! UE4 builds Detour with float dtReal, UE5 with double.
bool HasLargeWorldCoordinates(const FArchive& Ar) {
    return Ar.GetGame() >= EGame::GAME_UE5_0;
}

void SerializeReal(FArchive& Ar, dtReal& Value) {
    if (HasLargeWorldCoordinates(Ar)) {
        Ar << Value;
        return;
    }
    float Float = 0.0f;
    Ar << Float;
    Value = Float;
}

void SerializeReals(FArchive& Ar, dtReal* Values, int32 Count) {
    for (int32 Index = 0; Index < Count; Index++)
        SerializeReal(Ar, Values[Index]);
}

template <class T, class FSerializeElement>
void SerializeElements(FArchive& Ar, int32 Count, TArray<T>& Array, FSerializeElement SerializeElement) {
    if (Ar.IsError())
        return;
    Array.SetNum(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        SerializeElement(Array[Index]);
}

void SerializeTileSizeInfo(FArchive& Ar, uint32 NavMeshVersion, FDetourTileSizeInfo& SizeInfo, int32& ClusterConCount) {
    if (HasLargeWorldCoordinates(Ar)) {
        for (int32* Count : {&SizeInfo.VertCount, &SizeInfo.PolyCount, &SizeInfo.MaxLinkCount, &SizeInfo.DetailMeshCount, &SizeInfo.DetailVertCount,
                             &SizeInfo.DetailTriCount, &SizeInfo.BvNodeCount, &SizeInfo.OffMeshConCount, &SizeInfo.OffMeshSegConCount, &SizeInfo.ClusterCount})
            *Count = Ar.Read<uint16>();
    } else {
        SizeInfo.VertCount = Ar.ReadCount(12);
        SizeInfo.PolyCount = Ar.ReadCount(32);
        SizeInfo.MaxLinkCount = Ar.ReadCount(0);
        SizeInfo.DetailMeshCount = Ar.ReadCount(10);
        SizeInfo.DetailVertCount = Ar.ReadCount(12);
        SizeInfo.DetailTriCount = Ar.ReadCount(4);
        SizeInfo.BvNodeCount = Ar.ReadCount(16);
        SizeInfo.OffMeshConCount = Ar.ReadCount(36);
        SizeInfo.OffMeshSegConCount = Ar.ReadCount(60);
        SizeInfo.ClusterCount = Ar.ReadCount(12);
        if (NavMeshVersion < NAVMESHVER_CLUSTER_SIMPLIFIED)
            ClusterConCount = Ar.ReadCount(2);
    }
    SizeInfo.OffMeshBase = SizeInfo.DetailMeshCount;
}

void SerializeLargeWorldHeader(FArchive& Ar, uint32 NavMeshVersion, dtMeshHeader& H) {
    H.version = Ar.Read<uint16>();
    Ar << H.x << H.y;
    for (int32* Count : {&H.layer, &H.polyCount, &H.vertCount, &H.maxLinkCount, &H.detailMeshCount, &H.detailVertCount, &H.detailTriCount,
                         &H.bvNodeCount, &H.offMeshConCount, &H.offMeshBase})
        *Count = Ar.Read<uint16>();
    if (NavMeshVersion >= NAVMESHVER_TILE_RESOLUTIONS)
        Ar << H.resolution;
    SerializeReals(Ar, H.bmin, 3);
    SerializeReals(Ar, H.bmax, 3);
    for (int32* Count : {&H.clusterCount, &H.offMeshSegConCount, &H.offMeshSegPolyBase, &H.offMeshSegVertBase})
        *Count = Ar.Read<uint16>();
}

void SerializeHeader(FArchive& Ar, uint32 NavMeshVersion, dtMeshHeader& H) {
    if (HasLargeWorldCoordinates(Ar)) {
        SerializeLargeWorldHeader(Ar, NavMeshVersion, H);
        return;
    }
    Ar << H.magic << H.version << H.x << H.y;
    Ar << H.layer << H.userId << H.polyCount << H.vertCount;
    Ar << H.maxLinkCount << H.detailMeshCount << H.detailVertCount << H.detailTriCount;
    Ar << H.bvNodeCount << H.offMeshConCount << H.offMeshBase;
    SerializeReal(Ar, H.walkableHeight);
    SerializeReal(Ar, H.walkableRadius);
    SerializeReal(Ar, H.walkableClimb);
    SerializeReals(Ar, H.bmin, 3);
    SerializeReals(Ar, H.bmax, 3);
    SerializeReal(Ar, H.bvQuantFactor);
    Ar << H.clusterCount;
    if (NavMeshVersion < NAVMESHVER_CLUSTER_SIMPLIFIED)
        Ar << H.maxClusterCons;
    Ar << H.offMeshSegConCount << H.offMeshSegPolyBase << H.offMeshSegVertBase;
}

void SerializeTileVerts(FArchive& Ar, uint32 NavMeshVersion, const dtMeshHeader& Header, int32 Count, TArray<FVector>& Verts) {
    if (NavMeshVersion < NAVMESHVER_TILEVERT_ENCODING) {
        SerializeElements(Ar, Count, Verts, [&](FVector& V) {
            SerializeReal(Ar, V.X);
            SerializeReal(Ar, V.Y);
            SerializeReal(Ar, V.Z);
        });
        return;
    }
    SerializeElements(Ar, Count, Verts, [&](FVector& V) {
        float Local[3] = {};
        Ar << Local[0] << Local[1] << Local[2];
        V = FVector(Local[0] + Header.bmin[0], Local[1] + Header.bmin[1], Local[2] + Header.bmin[2]);
    });
}

void SerializeOffMeshConUserId(FArchive& Ar, dtOffMeshConnection& Conn) {
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::NavigationLinkID32To64, EGame::GAME_UE5_3)) {
        Ar << Conn.userId;
        return;
    }
    Conn.userId = Ar.Read<uint32>();
}

}

void FPImplRecastNavMesh::Serialize(FArchive& Ar, uint32 NavMeshVersion) {
    const int32 NumTiles = Ar.ReadCount(12);
    SerializeReals(Ar, Params.orig, 3);
    SerializeReal(Ar, Params.tileWidth);
    SerializeReal(Ar, Params.tileHeight);
    Ar << Params.maxTiles << Params.maxPolys;
    if (HasLargeWorldCoordinates(Ar)) {
        if (NavMeshVersion >= NAVMESHVER_TILE_RESOLUTIONS) {
            Ar << Params.walkableHeight << Params.walkableRadius << Params.walkableClimb;
            for (dtNavMeshResParams& ResolutionParams : Params.resolutionParams)
                Ar << ResolutionParams.bvQuantFactor;
        } else if (NavMeshVersion >= NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS) {
            Ar << Params.walkableHeight << Params.walkableRadius << Params.walkableClimb;
            Ar << Params.resolutionParams[static_cast<uint8>(ENavigationDataResolution::Default)].bvQuantFactor;
            for (dtNavMeshResParams& ResolutionParams : Params.resolutionParams)
                ResolutionParams = Params.resolutionParams[static_cast<uint8>(ENavigationDataResolution::Default)];
        } else {
            Params.walkableHeight = NavMeshOwner->AgentHeight;
            Params.walkableRadius = NavMeshOwner->AgentRadius;
            Params.walkableClimb = NavMeshOwner->AgentMaxStepHeight;
            for (dtNavMeshResParams& ResolutionParams : Params.resolutionParams)
                ResolutionParams.bvQuantFactor = 1.0f / NavMeshOwner->CellSize;
        }
    }
    for (int32 Index = 0; Index < NumTiles && !Ar.IsError(); Index++) {
        dtTileRef TileRef = std::numeric_limits<dtTileRef>::max();
        int32 TileDataSize = 0;
        Ar << TileRef << TileDataSize;
        if (TileRef == std::numeric_limits<dtTileRef>::max() || TileDataSize == 0)
            continue;
        FRecastMeshTile& Tile = Tiles[Tiles.AddDefaulted()];
        Tile.TileRef = TileRef;
        SerializeRecastMeshTile(Ar, NavMeshVersion, Tile);
        if (HasLargeWorldCoordinates(Ar))
            SerializeCompressedTileCacheData(Ar, Tile);
    }
}

void FPImplRecastNavMesh::SerializeRecastMeshTile(FArchive& Ar, uint32 NavMeshVersion, FRecastMeshTile& Tile) {
    FDetourTileSizeInfo SizeInfo;
    int32 ClusterConCount = 0;
    SerializeTileSizeInfo(Ar, NavMeshVersion, SizeInfo, ClusterConCount);
    const int32 PolyClusterCount = SizeInfo.OffMeshBase;
    if (Ar.IsError())
        return;
    SerializeHeader(Ar, NavMeshVersion, Tile.Header);
    SerializeTileVerts(Ar, NavMeshVersion, Tile.Header, SizeInfo.VertCount, Tile.NavVerts);
    SerializeElements(Ar, SizeInfo.PolyCount, Tile.NavPolys, [&](dtPoly& P) {
        Ar << P.firstLink;
        for (uint16& Vert : P.verts)
            Ar << Vert;
        for (uint16& Nei : P.neis)
            Ar << Nei;
        Ar << P.flags << P.vertCount << P.areaAndtype;
    });
    SerializeElements(Ar, SizeInfo.DetailMeshCount, Tile.DetailMeshes, [&](dtPolyDetail& DM) {
        if (HasLargeWorldCoordinates(Ar)) {
            DM.vertBase = Ar.Read<uint16>();
            DM.triBase = Ar.Read<uint16>();
        } else
            Ar << DM.vertBase << DM.triBase;
        Ar << DM.vertCount << DM.triCount;
    });
    SerializeTileVerts(Ar, NavMeshVersion, Tile.Header, SizeInfo.DetailVertCount, Tile.DetailVerts);
    SerializeRaw(Ar, Tile.DetailTris, SizeInfo.DetailTriCount * 4);
    if (Ar.GetGame() == EGame::GAME_MongilStarDive)
        Ar.SkipBytes(SizeInfo.PolyCount);
    SerializeElements(Ar, SizeInfo.BvNodeCount, Tile.BVTree, [&](dtBVNode& Node) {
        Ar << Node.bmin[0] << Node.bmin[1] << Node.bmin[2];
        Ar << Node.bmax[0] << Node.bmax[1] << Node.bmax[2];
        Ar << Node.i;
    });
    SerializeElements(Ar, SizeInfo.OffMeshConCount, Tile.OffMeshCons, [&](dtOffMeshConnection& Conn) {
        SerializeReals(Ar, Conn.pos, 6);
        SerializeReal(Ar, Conn.rad);
        Ar << Conn.poly << Conn.flags << Conn.side;
        SerializeOffMeshConUserId(Ar, Conn);
    });
    if (NavMeshVersion >= NAVMESHVER_OFFMESH_HEIGHT_BUG)
        for (dtOffMeshConnection& Conn : Tile.OffMeshCons)
            SerializeReal(Ar, Conn.height);
    SerializeElements(Ar, SizeInfo.OffMeshSegConCount, Tile.OffMeshSegs, [&](dtOffMeshSegmentConnection& Seg) {
        SerializeReals(Ar, Seg.startA, 3);
        SerializeReals(Ar, Seg.startB, 3);
        SerializeReals(Ar, Seg.endA, 3);
        SerializeReals(Ar, Seg.endB, 3);
        SerializeReal(Ar, Seg.rad);
        Ar << Seg.firstPoly << Seg.npolys << Seg.flags << Seg.userId;
    });
    SerializeElements(Ar, SizeInfo.ClusterCount, Tile.Clusters, [&](dtCluster& Cluster) {
        if (NavMeshVersion < NAVMESHVER_CLUSTER_SIMPLIFIED)
            Ar << Cluster.centerPoly << Cluster.firstLink << Cluster.numLinks;
        SerializeReals(Ar, Cluster.center, 3);
    });
    SerializeElements(Ar, PolyClusterCount, Tile.PolyClusters, [&](uint16& C) { Ar << C; });
    SerializeElements(Ar, ClusterConCount, Tile.ClusterCons, [&](uint16& C) { Ar << C; });
}

void FPImplRecastNavMesh::SerializeCompressedTileCacheData(FArchive& Ar, FRecastMeshTile& Tile) {
    int32 CompressedDataSizeNoHeader = 0;
    Ar << CompressedDataSizeNoHeader;
    if (Ar.IsError() || CompressedDataSizeNoHeader < 0)
        return;
    dtTileCacheLayerHeader& Header = Tile.TileCacheLayerHeader.emplace();
    Ar << Header.version << Header.tx << Header.ty << Header.tlayer;
    for (int32 Index = 0; Index < 3; Index++)
        Ar << Header.bmin[Index] << Header.bmax[Index];
    Ar << Header.hmin << Header.hmax << Header.width << Header.height;
    Ar << Header.minx << Header.maxx << Header.miny << Header.maxy;
    SerializeRaw(Ar, Tile.TileCacheCompressedData, CompressedDataSizeNoHeader);
}

}
