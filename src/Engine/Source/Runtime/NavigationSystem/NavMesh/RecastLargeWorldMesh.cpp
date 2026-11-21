#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastLargeWorldMesh.h"

#include <limits>

#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMesh.h"

namespace ue5parse::runtime {

namespace {

void ReadCacheLayer(FArchive& Ar, FRecastLargeWorldTile& Tile) {
    int32 Size = 0;
    Ar << Size;
    if (Size < 0 || Ar.IsError())
        return;
    auto& Layer = Tile.CacheLayer.emplace();
    Ar << Layer.Version << Layer.X << Layer.Y << Layer.Layer;
    Ar << Layer.BoundsMin.X << Layer.BoundsMax.X << Layer.BoundsMin.Y << Layer.BoundsMax.Y << Layer.BoundsMin.Z << Layer.BoundsMax.Z;
    Ar << Layer.HeightMin << Layer.HeightMax << Layer.Width << Layer.Height << Layer.MinX << Layer.MaxX << Layer.MinY << Layer.MaxY;
    if (Size > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FRecastTileCacheLayer.CompressedData");
        return;
    }
    SerializeRaw(Ar, Layer.CompressedData, Size);
}

}

void FRecastLargeWorldMesh::Serialize(FArchive& Ar, uint32 NavMeshVersion) {
    const int32 NumTiles = Ar.ReadCount(12);
    Ar << Params.Origin << Params.TileWidth << Params.TileHeight << Params.MaxTiles << Params.MaxPolys;
    if (NavMeshVersion >= NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS) {
        Ar << Params.WalkableHeight << Params.WalkableRadius << Params.WalkableClimb;
        if (NavMeshVersion >= NAVMESHVER_TILE_RESOLUTIONS)
            Ar << Params.Quantization[0] << Params.Quantization[1] << Params.Quantization[2];
        else {
            Ar << Params.Quantization[1];
            Params.Quantization[0] = Params.Quantization[2] = Params.Quantization[1];
        }
    }
    for (int32 Index = 0; Index < NumTiles && !Ar.IsError(); ++Index) {
        dtTileRef Ref = 0;
        int32 TileSize = 0;
        Ar << Ref << TileSize;
        if (Ref == std::numeric_limits<dtTileRef>::max() || TileSize == 0)
            continue;
        auto& Tile = Tiles[Tiles.AddDefaulted()];
        Tile.TileRef = Ref;
        Tile.TileDataSize = TileSize;
        Tile.Serialize(Ar, NavMeshVersion);
        if (!Ar.IsError())
            ReadCacheLayer(Ar, Tile);
    }
}

}
