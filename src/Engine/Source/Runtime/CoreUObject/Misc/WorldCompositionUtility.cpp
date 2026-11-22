#include "Engine/Source/Runtime/CoreUObject/Misc/WorldCompositionUtility.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

using V4 = EUnrealEngineObjectUE4Version;

void SkipString(FArchive& Ar) {
    FString Skipped;
    Ar << Skipped;
}

}

FArchive& operator<<(FArchive& Ar, FWorldTileLayer& D) {
    Ar << D.Name << D.Reserved0;
    if (Ar.GetGame() != EGame::GAME_PlayerUnknownsBattlegrounds)
        Ar << D.Reserved1;
    if (Ar.UEVer() >= V4::WORLD_LEVEL_INFO_UPDATED)
        Ar << D.StreamingDistance;
    if (Ar.UEVer() >= V4::WORLD_LAYER_ENABLE_DISTANCE_STREAMING)
        Ar << D.DistanceStreamingEnabled;
    if (Ar.GetGame() == EGame::GAME_ConanExilesEnhanced)
        Ar.SkipBytes(4);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FWorldTileLODInfo& D) {
    if (Ar.GetGame() == EGame::GAME_DuneAwakening || Ar.GetGame() == EGame::GAME_ConanExilesEnhanced)
        Ar.SkipBytes(4);
    Ar << D.RelativeStreamingDistance << D.Reserved0 << D.Reserved1 << D.Reserved2 << D.Reserved3;
    return Ar;
}

void FWorldTileInfo::StreamArchive(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::WorldCompositionTile3DOffset, EGame::GAME_UE4_20)) {
        FIntPoint Position2D;
        Ar << Position2D;
        Position = FIntVector(Position2D.X, Position2D.Y, 0);
    } else {
        Ar << Position;
    }
    Ar << Bounds;
    Ar << Layer;
    if (Ar.UEVer() >= V4::WORLD_LEVEL_INFO_UPDATED)
        Ar << bHideInTileView << ParentTilePackageName;
    if (Ar.UEVer() >= V4::WORLD_LEVEL_INFO_LOD_LIST)
        Ar << LODList;
    if (Ar.UEVer() >= V4::WORLD_LEVEL_INFO_ZORDER)
        Ar << ZOrder;
    if (Game == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(4);
    if (Game == EGame::GAME_DuneAwakening) {
        SkipString(Ar);
        SkipString(Ar);
    }
    if (Game == EGame::GAME_InfinityNikki) {
        Ar.SkipBytes(12);
        SkipString(Ar);
    }
    if (Game == EGame::GAME_PlayerUnknownsBattlegrounds || Game == EGame::GAME_Lego2KDrive)
        return;
    //! CUE4Parse reads eight more bytes in the cooked tiles before UE 5.0.
    if (Ar.UEVer() < EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES)
        Ar.SkipBytes(8);
}

}
