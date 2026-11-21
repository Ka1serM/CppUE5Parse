#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastLargeWorldMesh.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMesh.h"

namespace ue5parse::runtime {

namespace {

struct FDetourTileSizeInfo {
    uint16 Vertices = 0, Polygons = 0, MaxLinks = 0;
    uint16 DetailMeshes = 0, DetailVertices = 0, DetailTriangles = 0;
    uint16 BoundsNodes = 0, PointLinks = 0, SegmentLinks = 0, Clusters = 0;
};

template <class T, class FRead>
void ReadElements(FArchive& Ar, int32 Count, int32 MinBytes, TArray<T>& Out, FRead Read) {
    if (Ar.IsError())
        return;
    if (Count > Ar.GetRemaining() / MinBytes) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FRecastLargeWorldTile.Elements");
        return;
    }
    Out.SetNum(Count);
    for (auto& Element : Out) {
        Read(Element);
        if (Ar.IsError())
            return;
    }
}

void ReadHeader(FArchive& Ar, uint32 Version, FRecastLargeWorldTileHeader& Header) {
    Ar << Header.Version << Header.X << Header.Y << Header.Layer << Header.PolyCount << Header.VertCount;
    Ar << Header.MaxLinkCount << Header.DetailMeshCount << Header.DetailVertCount << Header.DetailTriCount;
    Ar << Header.BvNodeCount << Header.OffMeshConCount << Header.OffMeshBase;
    if (Version >= NAVMESHVER_TILE_RESOLUTIONS)
        Ar << Header.Resolution;
    Ar << Header.BoundsMin << Header.BoundsMax << Header.ClusterCount << Header.SegmentCount << Header.SegmentPolyBase << Header.SegmentVertBase;
}

void ReadVertex(FArchive& Ar, uint32 Version, const FVector& Origin, FVector& Vertex) {
    if (Version < NAVMESHVER_TILEVERT_ENCODING) {
        Ar << Vertex;
        return;
    }
    FVector3f Local;
    Ar << Local;
    Vertex = Origin + FVector(Local);
}

}

void FRecastLargeWorldTile::Serialize(FArchive& Ar, uint32 NavMeshVersion) {
    FDetourTileSizeInfo Counts;
    Ar << Counts.Vertices << Counts.Polygons << Counts.MaxLinks << Counts.DetailMeshes << Counts.DetailVertices << Counts.DetailTriangles;
    Ar << Counts.BoundsNodes << Counts.PointLinks << Counts.SegmentLinks << Counts.Clusters;
    ReadHeader(Ar, NavMeshVersion, Header);
    const auto Vertex = [&](FVector& V) { ReadVertex(Ar, NavMeshVersion, Header.BoundsMin, V); };
    const int32 VertexBytes = NavMeshVersion < NAVMESHVER_TILEVERT_ENCODING ? 24 : 12;
    ReadElements(Ar, Counts.Vertices, VertexBytes, Vertices, Vertex);
    ReadElements(Ar, Counts.Polygons, 32, Polygons, [&](dtPoly& Poly) {
        Ar << Poly.firstLink;
        for (auto& Index : Poly.verts)
            Ar << Index;
        for (auto& Neighbor : Poly.neis)
            Ar << Neighbor;
        Ar << Poly.flags << Poly.vertCount << Poly.areaAndtype;
    });
    ReadElements(Ar, Counts.DetailMeshes, 6, DetailMeshes, [&](dtPolyDetail& Detail) {
        Detail.vertBase = Ar.Read<uint16>();
        Detail.triBase = Ar.Read<uint16>();
        Ar << Detail.vertCount << Detail.triCount;
    });
    ReadElements(Ar, Counts.DetailVertices, VertexBytes, DetailVertices, Vertex);
    SerializeRaw(Ar, DetailTriangles, Counts.DetailTriangles * 4);
    ReadElements(Ar, Counts.BoundsNodes, 16, BoundsTree, [&](dtBVNode& Node) {
        Ar << Node.bmin[0] << Node.bmin[1] << Node.bmin[2] << Node.bmax[0] << Node.bmax[1] << Node.bmax[2] << Node.i;
    });
    const bool WideIds = Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::NavigationLinkID32To64,
                                             EGame::GAME_UE5_3);
    ReadElements(Ar, Counts.PointLinks, 64, PointLinks, [&](FRecastPointLink& Link) {
        Ar << Link.Start << Link.End << Link.Radius << Link.Poly << Link.Flags << Link.Side;
        if (WideIds)
            Ar << Link.UserId;
        else
            Link.UserId = Ar.Read<uint32>();
    });
    for (auto& Link : PointLinks)
        Ar << Link.Height;
    ReadElements(Ar, Counts.SegmentLinks, 112, SegmentLinks, [&](FRecastSegmentLink& Link) {
        Ar << Link.StartA << Link.StartB << Link.EndA << Link.EndB << Link.Radius << Link.FirstPoly << Link.NumPolys << Link.Flags << Link.UserId;
    });
    ReadElements(Ar, Counts.Clusters, 24, ClusterCenters, [&](FVector& Center) { Ar << Center; });
    SerializeRaw(Ar, PolygonClusters, Counts.DetailMeshes);
}

}
