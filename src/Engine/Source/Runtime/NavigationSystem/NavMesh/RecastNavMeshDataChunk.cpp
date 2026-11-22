#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMeshDataChunk.h"

#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastVersion.h"

namespace ue5parse::runtime {

void URecastNavMeshDataChunk::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() == EGame::GAME_OuterWorlds2)
        return;
    if (Ar.GetGame() < EGame::GAME_UE5_0) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "URecastNavMeshDataChunk.Serialize");
        return;
    }
    int32 NavMeshVersion = 0;
    Ar << NavMeshVersion;
    int64 RecastNavMeshSizeBytes = 0;
    const int64 RecastNavMeshSizePos = Ar.Tell();
    Ar << RecastNavMeshSizeBytes;
    if (Ar.IsError())
        return;
    if (!IsCompatibleNavMeshVersion(Ar.GetGame(), static_cast<uint32>(NavMeshVersion)) || RecastNavMeshSizeBytes <= 4) {
        Ar.Seek(RecastNavMeshSizePos + RecastNavMeshSizeBytes);
        return;
    }
    SerializeRecastData(Ar, static_cast<uint32>(NavMeshVersion));
}

void URecastNavMeshDataChunk::SerializeRecastData(FArchive& Ar, uint32 NavMeshVersion) {
    const int32 TileNum = Ar.ReadCount(4);
    for (int32 TileIdx = 0; TileIdx < TileNum && !Ar.IsError(); TileIdx++) {
        int32 TileDataSize = 0;
        Ar << TileDataSize;
        FRecastMeshTile& Tile = Tiles[Tiles.AddDefaulted()];
        FPImplRecastNavMesh::SerializeRecastMeshTile(Ar, NavMeshVersion, Tile);
        FPImplRecastNavMesh::SerializeCompressedTileCacheData(Ar, Tile);
    }
}

}
