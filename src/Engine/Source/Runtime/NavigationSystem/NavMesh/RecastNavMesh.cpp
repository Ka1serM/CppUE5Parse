#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMesh.h"

namespace ue5parse::runtime {

namespace {

//! NAVMESHVER_MIN_COMPATIBLE equals NAVMESHVER_LATEST of each engine version.
uint32 GetMinCompatibleNavMeshVersion(EGame Game) {
    return Game >= EGame::GAME_UE4_4 ? NAVMESHVER_CLUSTER_SIMPLIFIED : NAVMESHVER_64BIT;
}

}

void ARecastNavMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    if (Ar.GetGame() < EGame::GAME_UE4_3 || Ar.GetGame() >= EGame::GAME_UE4_5) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "ARecastNavMesh.Serialize");
        return;
    }
    Ar << NavMeshVersion;
    uint32 RecastNavMeshSizeBytes = 0;
    const int64 RecastNavMeshSizePos = Ar.Tell();
    Ar << RecastNavMeshSizeBytes;
    if (Ar.IsError())
        return;
    if (NavMeshVersion < GetMinCompatibleNavMeshVersion(Ar.GetGame()) || RecastNavMeshSizeBytes <= 4) {
        Ar.Seek(RecastNavMeshSizePos + RecastNavMeshSizeBytes);
        return;
    }
    RecastNavMeshImpl = std::make_unique<FPImplRecastNavMesh>();
    RecastNavMeshImpl->Serialize(Ar, NavMeshVersion);
}

}
