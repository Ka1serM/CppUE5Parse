#include "CppUE5Parse/Core/Versions.h"
#include "Test.h"

using ue5parse::EGame;
using ue5parse::EUnrealEngineObjectUE5Version;
using ue5parse::getPackageFileVersion;
using ue5parse::parseGame;

TEST(parseGameNamesAndVersions) {
    CHECK(parseGame("4.0") == EGame::GAME_UE4_0);
    CHECK(parseGame("4.27") == EGame::GAME_UE4_27);
    CHECK(parseGame("5.3.2") == EGame::GAME_UE5_3);
    CHECK(parseGame("GAME_UE5_9") == EGame::GAME_UE5_9);
    CHECK(parseGame("GAME_BlackMythWukong") == EGame::GAME_BlackMythWukong);
    CHECK(parseGame("GAME_Unknown") == std::nullopt);
    CHECK(parseGame("4.28") == std::nullopt);
    CHECK(parseGame("3.5") == std::nullopt);
    CHECK(parseGame("5.") == std::nullopt);
    CHECK(parseGame("5") == std::nullopt);
    CHECK(parseGame("5.3x") == std::nullopt);
}

TEST(gamePresetPackageVersions) {
    CHECK(getPackageFileVersion(EGame::GAME_UE5_0) == EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES);
    CHECK(getPackageFileVersion(EGame::GAME_TheFirstDescendant) == EUnrealEngineObjectUE5Version::PAYLOAD_TOC);
    CHECK(getPackageFileVersion(EGame::GAME_BlackMythWukong) == getPackageFileVersion(EGame::GAME_UE5_0));
}
