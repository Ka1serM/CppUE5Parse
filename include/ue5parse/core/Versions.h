#pragma once

#include <cstdint>

namespace ue5parse {

// The engine release that cooked the game. Cooked Zen packages carry no version, so the layout of their headers and
// of many native structs comes from here (design §9). The values compare in release order.
enum class EngineVersion : std::uint16_t {
    UE5_0 = 500,
    UE5_1 = 501,
    UE5_2 = 502,
    UE5_3 = 503,
    UE5_4 = 504,
    UE5_5 = 505,
    UE5_6 = 506,
    UE5_7 = 507,
    UE5_8 = 508,
};

constexpr EngineVersion latestEngineVersion = EngineVersion::UE5_8;

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

// The latest UE5 object version of each release (from ObjectVersion.h of the release tags).
constexpr EUnrealEngineObjectUE5Version getLatestObjectVersion(EngineVersion engine) {
    using V = EUnrealEngineObjectUE5Version;
    switch (engine) {
    case EngineVersion::UE5_0: return V::LARGE_WORLD_COORDINATES;
    case EngineVersion::UE5_1: return V::ADD_SOFTOBJECTPATH_LIST;
    case EngineVersion::UE5_2: return V::DATA_RESOURCES;
    case EngineVersion::UE5_3: return V::DATA_RESOURCES;
    case EngineVersion::UE5_4: return V::PROPERTY_TAG_COMPLETE_TYPE_NAME;
    case EngineVersion::UE5_5: return V::ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES;
    case EngineVersion::UE5_6: return V::OS_SUB_OBJECT_SHADOW_SERIALIZATION;
    case EngineVersion::UE5_7: return V::IMPORT_TYPE_HIERARCHIES;
    case EngineVersion::UE5_8: return V::IMPORT_TYPE_HIERARCHIES;
    }
    return V::IMPORT_TYPE_HIERARCHIES;
}

}
