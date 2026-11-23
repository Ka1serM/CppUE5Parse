#include "Engine/Source/Runtime/NavigationSystem/NavMesh/RecastNavMesh.h"

namespace ue5parse::runtime {

void ARecastNavMesh::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    if (Ar.GetGame() == EGame::GAME_TheFirstDescendant && GetClass()->GetName().Contains(FString("M1RecastNavMesh")))
        return;
    if (Ar.GetGame() < EGame::GAME_UE4_3 || (Ar.GetGame() >= EGame::GAME_UE4_5 && Ar.GetGame() < EGame::GAME_UE5_0)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "ARecastNavMesh.Serialize");
        return;
    }
    Ar << NavMeshVersion;
    uint32 RecastNavMeshSizeBytes = 0;
    const int64 RecastNavMeshSizePos = Ar.Tell();
    Ar << RecastNavMeshSizeBytes;
    if (Ar.IsError())
        return;
    if (!IsCompatibleNavMeshVersion(Ar.GetGame(), NavMeshVersion) || RecastNavMeshSizeBytes <= 4) {
        Ar.Seek(RecastNavMeshSizePos + RecastNavMeshSizeBytes);
        return;
    }
    RecastNavMeshImpl = std::make_unique<FPImplRecastNavMesh>(this);
    RecastNavMeshImpl->Serialize(Ar, NavMeshVersion);
}

}
