#pragma once

#include <optional>

#include "Engine/Source/Runtime/NavigationSystem/NavMesh/PImplRecastNavMesh.h"

namespace ue5parse::runtime {

struct FRecastLargeWorldParams {
    FVector Origin;
    double TileWidth = 0, TileHeight = 0;
    int32 MaxTiles = 0, MaxPolys = 0;
    double WalkableHeight = 0, WalkableRadius = 0, WalkableClimb = 0;
    double Quantization[3]{};
};

struct FRecastLargeWorldTileHeader {
    uint16 Version = 0;
    int32 X = 0, Y = 0;
    uint16 Layer = 0, PolyCount = 0, VertCount = 0, MaxLinkCount = 0;
    uint16 DetailMeshCount = 0, DetailVertCount = 0, DetailTriCount = 0;
    uint16 BvNodeCount = 0, OffMeshConCount = 0, OffMeshBase = 0;
    uint8 Resolution = 0;
    FVector BoundsMin, BoundsMax;
    uint16 ClusterCount = 0, SegmentCount = 0, SegmentPolyBase = 0, SegmentVertBase = 0;
};

struct FRecastPointLink {
    FVector Start, End;
    double Radius = 0, Height = 0;
    uint16 Poly = 0;
    uint8 Flags = 0, Side = 0;
    uint64 UserId = 0;
};

struct FRecastSegmentLink {
    FVector StartA, StartB, EndA, EndB;
    double Radius = 0;
    uint16 FirstPoly = 0;
    uint8 NumPolys = 0, Flags = 0;
    uint32 UserId = 0;
};

struct FRecastTileCacheLayer {
    int16 Version = 0;
    int32 X = 0, Y = 0, Layer = 0;
    FVector BoundsMin, BoundsMax;
    uint16 HeightMin = 0, HeightMax = 0, Width = 0, Height = 0;
    uint16 MinX = 0, MaxX = 0, MinY = 0, MaxY = 0;
    TArray<uint8> CompressedData;
};

struct FRecastLargeWorldTile {
    dtTileRef TileRef = 0;
    int32 TileDataSize = 0;
    FRecastLargeWorldTileHeader Header;
    TArray<FVector> Vertices, DetailVertices, ClusterCenters;
    TArray<dtPoly> Polygons;
    TArray<dtPolyDetail> DetailMeshes;
    TArray<uint8> DetailTriangles;
    TArray<dtBVNode> BoundsTree;
    TArray<FRecastPointLink> PointLinks;
    TArray<FRecastSegmentLink> SegmentLinks;
    TArray<uint16> PolygonClusters;
    std::optional<FRecastTileCacheLayer> CacheLayer;

    void Serialize(FArchive& Ar, uint32 NavMeshVersion);
};

struct FRecastLargeWorldMesh {
    FRecastLargeWorldParams Params;
    TArray<FRecastLargeWorldTile> Tiles;

    void Serialize(FArchive& Ar, uint32 NavMeshVersion);
};

}
