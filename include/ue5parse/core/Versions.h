#pragma once

#include <cstdint>

namespace ue5parse {

// The game that cooked the data, as CUE4Parse's EGame (design §9). The provider has it, and archives copy it. Cooked Zen
// packages carry no version, so the layout of their headers and of many native structs comes from here. A value is
// its engine release plus an offset in the low four bits, so a game compares in release order with the plain
// releases: GAME_Valorant >= GAME_UE5_3 and < GAME_UE5_4. Game-specific code compares for equality:
// `Ar.GetGame() == EGame::GAME_Valorant`.
enum class EGame : std::uint32_t {
    GAME_UE5_0 = 0x2000000 + (0 << 4),
    GAME_UE5_1 = 0x2000000 + (1 << 4),
    GAME_UE5_2 = 0x2000000 + (2 << 4),
    GAME_UE5_3 = 0x2000000 + (3 << 4),
    GAME_Valorant = GAME_UE5_3 + 1,   // VALORANT: UE 5.3 with Riot's changes
    GAME_UE5_4 = 0x2000000 + (4 << 4),
    GAME_UE5_5 = 0x2000000 + (5 << 4),
    GAME_UE5_6 = 0x2000000 + (6 << 4),
    GAME_UE5_7 = 0x2000000 + (7 << 4),
    GAME_UE5_8 = 0x2000000 + (8 << 4),
    GAME_UE5_LATEST = GAME_UE5_8,
};

// EUnrealEngineObjectUE5Version: the package file version of UE5 data. Versioned packages store it; unversioned
// (cooked) packages have the latest one of their engine release.
enum class EUnrealEngineObjectUE5Version : std::int32_t {
    INITIAL_VERSION = 1000,
    NAMES_REFERENCED_FROM_EXPORT_DATA,
    PAYLOAD_TOC,
    OPTIONAL_RESOURCES,
    LARGE_WORLD_COORDINATES,
    REMOVE_OBJECT_EXPORT_PACKAGE_GUID,
    TRACK_OBJECT_EXPORT_IS_INHERITED,
    FSOFTOBJECTPATH_REMOVE_ASSET_PATH_FNAMES,
    ADD_SOFTOBJECTPATH_LIST,
    DATA_RESOURCES,
    SCRIPT_SERIALIZATION_OFFSET,
    PROPERTY_TAG_EXTENSION_AND_OVERRIDABLE_SERIALIZATION,
    PROPERTY_TAG_COMPLETE_TYPE_NAME,
    ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES,
    METADATA_SERIALIZATION_OFFSET,
    VERSE_CELLS,
    PACKAGE_SAVED_HASH,
    OS_SUB_OBJECT_SHADOW_SERIALIZATION,
    IMPORT_TYPE_HIERARCHIES,
};

// The latest UE5 object version of the game's release (from ObjectVersion.h of the release tags).
constexpr EUnrealEngineObjectUE5Version getLatestObjectVersion(EGame game) {
    using V = EUnrealEngineObjectUE5Version;
    if (game >= EGame::GAME_UE5_7) return V::IMPORT_TYPE_HIERARCHIES;
    if (game >= EGame::GAME_UE5_6) return V::OS_SUB_OBJECT_SHADOW_SERIALIZATION;
    if (game >= EGame::GAME_UE5_5) return V::ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES;
    if (game >= EGame::GAME_UE5_4) return V::PROPERTY_TAG_COMPLETE_TYPE_NAME;
    if (game >= EGame::GAME_UE5_2) return V::DATA_RESOURCES;
    if (game >= EGame::GAME_UE5_1) return V::ADD_SOFTOBJECTPATH_LIST;
    return V::LARGE_WORLD_COORDINATES;
}

}
