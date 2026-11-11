#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace ue5parse {

enum class EGame : std::uint32_t {
    GAME_UE5_0 = 0x2000000 + (0 << 4),
    GAME_UE5_1 = 0x2000000 + (1 << 4),
    GAME_UE5_2 = 0x2000000 + (2 << 4),
    GAME_UE5_3 = 0x2000000 + (3 << 4),
    GAME_Valorant = GAME_UE5_3 + 1,
    GAME_UE5_4 = 0x2000000 + (4 << 4),
    GAME_UE5_5 = 0x2000000 + (5 << 4),
    GAME_UE5_6 = 0x2000000 + (6 << 4),
    GAME_UE5_7 = 0x2000000 + (7 << 4),
    GAME_UE5_8 = 0x2000000 + (8 << 4),
    GAME_UE5_LATEST = GAME_UE5_8,
};

//! The name is the enum name, as CUE4Parse writes it; settings files save it.
struct GamePreset {
    EGame game;
    std::string_view name;
    std::string_view title;
};

inline constexpr std::array gamePresets{
    GamePreset{EGame::GAME_UE5_0, "GAME_UE5_0", "Unreal Engine 5.0"},
    GamePreset{EGame::GAME_UE5_1, "GAME_UE5_1", "Unreal Engine 5.1"},
    GamePreset{EGame::GAME_UE5_2, "GAME_UE5_2", "Unreal Engine 5.2"},
    GamePreset{EGame::GAME_UE5_3, "GAME_UE5_3", "Unreal Engine 5.3"},
    GamePreset{EGame::GAME_Valorant, "GAME_Valorant", "Valorant"},
    GamePreset{EGame::GAME_UE5_4, "GAME_UE5_4", "Unreal Engine 5.4"},
    GamePreset{EGame::GAME_UE5_5, "GAME_UE5_5", "Unreal Engine 5.5"},
    GamePreset{EGame::GAME_UE5_6, "GAME_UE5_6", "Unreal Engine 5.6"},
    GamePreset{EGame::GAME_UE5_7, "GAME_UE5_7", "Unreal Engine 5.7"},
    GamePreset{EGame::GAME_UE5_8, "GAME_UE5_8", "Unreal Engine 5.8"},
};

constexpr std::optional<EGame> gameOfName(const std::string_view name) {
    const auto found = std::ranges::find(gamePresets, name, &GamePreset::name);
    if (found == gamePresets.end())
        return std::nullopt;
    return found->game;
}

constexpr const GamePreset& presetOf(const EGame game) {
    return *std::ranges::find(gamePresets, game, &GamePreset::game);
}

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

constexpr EUnrealEngineObjectUE5Version getLatestObjectVersion(EGame game) {
    using V = EUnrealEngineObjectUE5Version;
    if (game >= EGame::GAME_UE5_7)
        return V::IMPORT_TYPE_HIERARCHIES;
    if (game >= EGame::GAME_UE5_6)
        return V::OS_SUB_OBJECT_SHADOW_SERIALIZATION;
    if (game >= EGame::GAME_UE5_5)
        return V::ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES;
    if (game >= EGame::GAME_UE5_4)
        return V::PROPERTY_TAG_COMPLETE_TYPE_NAME;
    if (game >= EGame::GAME_UE5_2)
        return V::DATA_RESOURCES;
    if (game >= EGame::GAME_UE5_1)
        return V::ADD_SOFTOBJECTPATH_LIST;
    return V::LARGE_WORLD_COORDINATES;
}

}
