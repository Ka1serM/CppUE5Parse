#include <cstddef>
#include <span>

#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Test.h"

using ue5parse::EGame;
using ue5parse::EUnrealEngineObjectUE4Version;
using ue5parse::EUnrealEngineObjectUE5Version;
using ue5parse::FGuid;
using ue5parse::getPackageFileVersion;
using ue5parse::parseGame;
using namespace ue5parse::runtime;

namespace {

void Configure(FArchive& Ar, EGame Game, bool bUnversioned) {
    Ar.SetGame(Game);
    Ar.SetUnversioned(bUnversioned);
    if (bUnversioned)
        Ar.SetCustomVersions(GetUnversionedCustomVersions(Game));
}

bool AtLeast(EGame Game, bool bUnversioned, const FGuid& Key, int32 Version, EGame Since) {
    FMemoryReaderView Ar{std::span<const std::byte>{}};
    Configure(Ar, Game, bUnversioned);
    return Ar.CustomVerAtLeast(Key, Version, Since);
}

int32 UnversionedCustomVer(EGame Game, const FGuid& Key) {
    FMemoryReaderView Ar{std::span<const std::byte>{}};
    Configure(Ar, Game, true);
    return Ar.CustomVer(Key);
}

}

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
    CHECK(parseGame("GAME_TEKKEN7") == EGame::GAME_TEKKEN7);
    CHECK(parseGame("GAME_Paragon") == EGame::GAME_Paragon);
    CHECK(parseGame("GAME_AssaultFireFuture") == EGame::GAME_AssaultFireFuture);
    CHECK(parseGame("GAME_DarkPicturesAnthologyTheDevilinMe") == EGame::GAME_DarkPicturesAnthologyTheDevilinMe);
    CHECK(parseGame("GAME_DeltaForce") == EGame::GAME_DeltaForce);
    CHECK(parseGame("GAME_Aion2") == EGame::GAME_Aion2);

    //! The per-game custom versions are the values of CUE4Parse's FxxxVersion.Get.
    CHECK(UnversionedCustomVer(EGame::GAME_TEKKEN7, FEditorObjectVersion::GUID) == 7);
    CHECK(UnversionedCustomVer(EGame::GAME_TEKKEN7, FFrameworkObjectVersion::GUID) == 14);
    CHECK(UnversionedCustomVer(EGame::GAME_TEKKEN7, FRenderingObjectVersion::GUID) == 9);
    CHECK(UnversionedCustomVer(EGame::GAME_Paragon, FEditorObjectVersion::GUID) == 22);
    CHECK(UnversionedCustomVer(EGame::GAME_Paragon, FSkeletalMeshCustomVersion::GUID) == 12);
    CHECK(UnversionedCustomVer(EGame::GAME_AssaultFireFuture, FReleaseObjectVersion::GUID) == 30);
    CHECK(UnversionedCustomVer(EGame::GAME_DarkPicturesAnthologyManofMedan, FReleaseObjectVersion::GUID) == 44);
    CHECK(UnversionedCustomVer(EGame::GAME_DarkPicturesAnthologyLittleHope, FReleaseObjectVersion::GUID) == 44);
    CHECK(UnversionedCustomVer(EGame::GAME_DarkPicturesAnthologyTheDevilinMe, FReleaseObjectVersion::GUID) == 44);
    CHECK(UnversionedCustomVer(EGame::GAME_DeltaForce, FAnimObjectVersion::GUID) == 2);
    CHECK(UnversionedCustomVer(EGame::GAME_DeltaForce, FSequencerObjectVersion::GUID) == 11);
    CHECK(UnversionedCustomVer(EGame::GAME_BlackMythWukong, FPropertyBagCustomVersion::GUID) == 2);
    CHECK(UnversionedCustomVer(EGame::GAME_BlackMythWukong, FUE5MainStreamObjectVersion::GUID) == 61);
    CHECK(UnversionedCustomVer(EGame::GAME_BlackMythWukong, FUE5ReleaseStreamObjectVersion::GUID) == 38);
    CHECK(UnversionedCustomVer(EGame::GAME_Aion2, FFortniteMainBranchObjectVersion::GUID) == 120);
    CHECK(UnversionedCustomVer(EGame::GAME_Aion2, FRigVMObjectVersion::GUID) == 7);
    CHECK(UnversionedCustomVer(EGame::GAME_UE4_27, FReleaseObjectVersion::GUID) == -1);

    //! A game override wins over the engine threshold, and the other keys keep the engine thresholds.
    CHECK(!AtLeast(EGame::GAME_DeltaForce, true, FAnimObjectVersion::GUID, FAnimObjectVersion::IncreaseBoneIndexLimitPerChunk, EGame::GAME_UE4_21));
    CHECK(AtLeast(EGame::GAME_DarkPicturesAnthologyLittleHope, true, FReleaseObjectVersion::GUID, FReleaseObjectVersion::PropertiesSerializeRepCondition,
                  EGame::GAME_UE5_9));
    CHECK(AtLeast(EGame::GAME_TEKKEN7, true, FAnimPhysObjectVersion::GUID, FAnimPhysObjectVersion::RemoveUIDFromSmartNameSerialize, EGame::GAME_UE4_14));

    //! An unversioned package resolves the custom versions by game, a versioned one without the key at version -1.
    CHECK(!AtLeast(EGame::GAME_UE4_20, true, FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_21));
    CHECK(AtLeast(EGame::GAME_UE4_21, true, FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_21));
    CHECK(!AtLeast(EGame::GAME_UE4_27, false, FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_21));
    FMemoryReaderView Tagged{std::span<const std::byte>{}};
    Configure(Tagged, EGame::GAME_UE4_21, true);
    Tagged.SetUseUnversionedPropertySerialization(false);
    CHECK(Tagged.CustomVerAtLeast(FAnimObjectVersion::GUID, FAnimObjectVersion::StoreMarkerNamesOnSkeleton, EGame::GAME_UE4_21));
}

TEST(gamePresetPackageVersions) {
    CHECK(getPackageFileVersion(EGame::GAME_UE5_0) == EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES);
    CHECK(getPackageFileVersion(EGame::GAME_TheFirstDescendant) == EUnrealEngineObjectUE5Version::PAYLOAD_TOC);
    CHECK(getPackageFileVersion(EGame::GAME_BlackMythWukong) == getPackageFileVersion(EGame::GAME_UE5_0));
    CHECK(getPackageFileVersion(EGame::GAME_UE5_EA) == EUnrealEngineObjectUE5Version::PAYLOAD_TOC);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_1) == EUnrealEngineObjectUE5Version::ADD_SOFTOBJECTPATH_LIST);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_1) >= EUnrealEngineObjectUE5Version::FSOFTOBJECTPATH_REMOVE_ASSET_PATH_FNAMES);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_0) < EUnrealEngineObjectUE5Version::FSOFTOBJECTPATH_REMOVE_ASSET_PATH_FNAMES);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_3) == EUnrealEngineObjectUE5Version::DATA_RESOURCES);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_4) == EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_5) == EUnrealEngineObjectUE5Version::ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_6).fileVersionUE5 == 1017);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_7) == EUnrealEngineObjectUE5Version::AUTOMATIC_VERSION);
    CHECK(getPackageFileVersion(EGame::GAME_UE4_0).fileVersionUE4 == 342);
    CHECK(getPackageFileVersion(EGame::GAME_UE4_10).fileVersionUE4 == 482);
    CHECK(getPackageFileVersion(EGame::GAME_UE4_26).fileVersionUE4 == 522);
    CHECK(getPackageFileVersion(EGame::GAME_UE4_27) == EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    CHECK(getPackageFileVersion(EGame::GAME_TEKKEN7) == getPackageFileVersion(EGame::GAME_UE4_14));
    CHECK(getPackageFileVersion(EGame::GAME_DeltaForce) == getPackageFileVersion(EGame::GAME_UE4_27));
}

TEST(customGamePresets) {
    CHECK(parseGame("GAME_ArkSurvivalEvolved") == EGame::GAME_ArkSurvivalEvolved);
    CHECK(parseGame("Game_StyxShardsofDarkness") == EGame::Game_StyxShardsofDarkness);
    CHECK(parseGame("GAME_WutheringWavesFastGeo") == EGame::GAME_WutheringWavesFastGeo);
    CHECK(parseGame("GAME_UE5_EA") == EGame::GAME_UE5_EA);
    CHECK(EGame::GAME_ArkSurvivalEvolved > EGame::GAME_UE4_5);
    CHECK(EGame::GAME_ArkSurvivalEvolved < EGame::GAME_UE4_6);
    CHECK(EGame::GAME_WutheringWavesFastGeo > EGame::GAME_UE5_8);
    CHECK(EGame::GAME_WutheringWavesFastGeo < EGame::GAME_UE5_9);
    CHECK(EGame::GAME_BlackMythWukong > EGame::GAME_UE5_0);
    CHECK(getPackageFileVersion(EGame::GAME_UE5_EA) == getPackageFileVersion(EGame::GAME_TheFirstDescendant));
    CHECK(getPackageFileVersion(EGame::GAME_ArkSurvivalEvolved) == getPackageFileVersion(EGame::GAME_UE4_5));
    CHECK(getPackageFileVersion(EGame::GAME_SeaOfThieves) == getPackageFileVersion(EGame::GAME_UE4_10));
}
