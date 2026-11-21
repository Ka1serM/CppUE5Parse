#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMesh.h"

namespace ue5parse::runtime {

namespace {

//! UE 4.3 and 4.4 each accept only their latest navigation version.
uint32 GetMinCompatibleNavMeshVersion(EGame Game) {
    return Game >= EGame::GAME_UE4_4 ? NAVMESHVER_CLUSTER_SIMPLIFIED : NAVMESHVER_64BIT;
}

uint32 GetLatestLargeWorldNavMeshVersion(EGame Game) {
    if (Game >= EGame::GAME_UE5_8)
        return NAVMESHVER_TILEVERT_ENCODING;
    if (Game >= EGame::GAME_UE5_7)
        return NAVMESHVER_NAVLINK_JUMP_CONFIGS;
    return NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT;
}

}

void ARecastNavMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    const bool LargeWorld = Ar.GetGame() >= EGame::GAME_UE5_3;
    if (!LargeWorld && (Ar.GetGame() < EGame::GAME_UE4_3 || Ar.GetGame() >= EGame::GAME_UE4_5)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "ARecastNavMesh.Serialize");
        return;
    }
    Ar << NavMeshVersion;
    uint32 RecastNavMeshSizeBytes = 0;
    const int64 RecastNavMeshSizePos = Ar.Tell();
    Ar << RecastNavMeshSizeBytes;
    if (Ar.IsError())
        return;
    if (LargeWorld) {
        const uint32 Latest = GetLatestLargeWorldNavMeshVersion(Ar.GetGame());
        if (RecastNavMeshSizeBytes < 4 || RecastNavMeshSizeBytes - 4 > Ar.GetRemaining()) {
            Ar.SetError(ErrorCode::InvalidData, "ARecastNavMesh.Size");
            return;
        }
        const int64 End = RecastNavMeshSizePos + RecastNavMeshSizeBytes;
        if (NavMeshVersion < NAVMESHVER_LWCOORDS_OPTIMIZATION || NavMeshVersion > Latest || RecastNavMeshSizeBytes == 4) {
            Ar.Seek(End);
            return;
        }
        FMemoryReaderView Payload(Ar.ReadView(End - Ar.Tell()));
        if (Ar.IsError())
            return;
        Payload.CopyStateFrom(Ar);
        LargeWorldMesh = std::make_unique<FRecastLargeWorldMesh>();
        LargeWorldMesh->Serialize(Payload, NavMeshVersion);
        if (Payload.IsError())
            Ar.SetError(Payload.GetError().code, Payload.GetError().where);
        else if (!Payload.AtEnd())
            Ar.SetError(ErrorCode::InvalidData, "ARecastNavMesh.PayloadSize");
        return;
    }
    if (NavMeshVersion < GetMinCompatibleNavMeshVersion(Ar.GetGame()) || RecastNavMeshSizeBytes <= 4) {
        Ar.Seek(RecastNavMeshSizePos + RecastNavMeshSizeBytes);
        return;
    }
    RecastNavMeshImpl = std::make_unique<FPImplRecastNavMesh>();
    RecastNavMeshImpl->Serialize(Ar, NavMeshVersion);
}

}
