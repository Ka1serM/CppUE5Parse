#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "CppUE5Parse/Core/Versions.h"

namespace ue5parse::runtime {

constexpr uint32 NAVMESHVER_64BIT = 9;
constexpr uint32 NAVMESHVER_CLUSTER_SIMPLIFIED = 10;
constexpr uint32 NAVMESHVER_OFFMESH_HEIGHT_BUG = 11;
constexpr uint32 NAVMESHVER_LWCOORDS_OPTIMIZATION = 20;
constexpr uint32 NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS = 21;
constexpr uint32 NAVMESHVER_TILE_RESOLUTIONS = 23;
constexpr uint32 NAVMESHVER_TILE_RESOLUTIONS_CELLHEIGHT = 24;
constexpr uint32 NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT = 26;
constexpr uint32 NAVMESHVER_NAVLINK_JUMP_CONFIGS = 27;
constexpr uint32 NAVMESHVER_TILEVERT_ENCODING = 28;

//! UE4 has no NAVMESHVER_LATEST check on load, and its NAVMESHVER_MIN_COMPATIBLE equals its NAVMESHVER_LATEST.
inline uint32 GetMinCompatibleNavMeshVersion(EGame Game) {
    if (Game >= EGame::GAME_UE5_0)
        return NAVMESHVER_LWCOORDS_OPTIMIZATION;
    return Game >= EGame::GAME_UE4_4 ? NAVMESHVER_CLUSTER_SIMPLIFIED : NAVMESHVER_64BIT;
}

inline uint32 GetLatestNavMeshVersion(EGame Game) {
    if (Game >= EGame::GAME_UE5_8)
        return NAVMESHVER_TILEVERT_ENCODING;
    if (Game >= EGame::GAME_UE5_7)
        return NAVMESHVER_NAVLINK_JUMP_CONFIGS;
    if (Game >= EGame::GAME_UE5_3)
        return NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT;
    if (Game >= EGame::GAME_UE5_2)
        return NAVMESHVER_TILE_RESOLUTIONS_CELLHEIGHT;
    return NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS;
}

inline bool IsCompatibleNavMeshVersion(EGame Game, uint32 NavMeshVersion) {
    if (NavMeshVersion < GetMinCompatibleNavMeshVersion(Game))
        return false;
    return Game < EGame::GAME_UE5_0 || NavMeshVersion <= GetLatestNavMeshVersion(Game);
}

}
