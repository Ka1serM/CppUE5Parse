#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/GameTypes/AceCombat7/AceCombat7Decrypt.h"
#include "CppUE5Parse/GameTypes/ArenaBreakoutInfinite/ArenaBreakoutInfiniteAes.h"
#include "CppUE5Parse/GameTypes/FinalFantasy7Rebirth/FinalFantasy7RebirthMesh.h"
#include "CppUE5Parse/GameTypes/InZOI/InZOIPakInfo.h"
#include "CppUE5Parse/GameTypes/PropertyTypeGuesses.h"
#include "CppUE5Parse/GameTypes/RocoKingdomWorld/RocoKingdomWorldMorphTarget.h"
#include "CppUE5Parse/GameTypes/SupermassiveGames/DarkPicturesMorphTarget.h"
#include "CppUE5Parse/Package/LegacyPackage.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "GameTypes/FinalFantasy7Rebirth/MemoryMappedAsset.h"
#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"
#include "GameTypes/Subnautica2/MercunaNavDataChunk.h"
#include "Test.h"

namespace {

using ue5parse::ArenaBreakoutInfiniteFile;

std::vector<std::byte> pattern(std::size_t size, std::size_t multiplier, std::size_t offset) {
    std::vector<std::byte> bytes(size);
    for (std::size_t i = 0; i < size; i++)
        bytes[i] = static_cast<std::byte>(i * multiplier + offset);
    return bytes;
}

std::vector<std::byte> fromHex(std::string_view text) {
    std::vector<std::byte> bytes(text.size() / 2);
    for (std::size_t i = 0; i < bytes.size(); i++)
        bytes[i] = static_cast<std::byte>(std::stoi(std::string(text.substr(i * 2, 2)), nullptr, 16));
    return bytes;
}

void writeLittleEndian(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value, std::size_t size) {
    for (std::size_t i = 0; i < size; i++)
        bytes[offset + i] = static_cast<std::byte>(value >> (i * 8));
}

void appendWords(std::vector<std::byte>& bytes, std::initializer_list<std::uint32_t> words) {
    for (std::uint32_t word : words)
        for (std::size_t i = 0; i < 4; i++)
            bytes.push_back(static_cast<std::byte>(word >> (i * 8)));
}

void appendHalves(std::vector<std::byte>& bytes, std::initializer_list<std::uint16_t> halves) {
    for (std::uint16_t half : halves) {
        bytes.push_back(static_cast<std::byte>(half));
        bytes.push_back(static_cast<std::byte>(half >> 8));
    }
}

bool deltaEquals(const ue5parse::DecodedMorphTargetDelta& delta, std::array<float, 3> position, std::array<float, 3> tangent, std::uint32_t index) {
    return delta.positionDelta == position && delta.tangentZDelta == tangent && delta.sourceIndex == index;
}

struct PackageVector {
    std::uint32_t magic;
    std::string_view expected;
};

//! Outputs of CUE4Parse ABIDecryption.AbiDecryptPackageSummary for 80 bytes of i * 11 + 5 with the magic and a 32 byte summary.
constexpr PackageVector packageVectors[] = {
    {0x03000337, "C1832A9EF9FFFFFF835CBBC0C0C5694EF69AA14E8C7681CB6C11055C2D7F431006B2B889D3278FB9BDC8D3DEE9F4FF0A15202B36414C57626D78838E99A4AFBAC5D0DBE6F1FC07121D28333E49545F6A"},
    {0x03000338, "C1832A9EF9FFFFFFFAA73A8D26722D02BCA826810E8CF7FBB3A273E077C7F1DE7BD1BDEBCF103975BDC8D3DEE9F4FF0A15202B36414C57626D78838E99A4AFBAC5D0DBE6F1FC07121D28333E49545F6A"},
    {0x03000339, "C1832A9EF9FFFFFF0F9A280F42CB4CAB5DC210335E91DA6378F0B595D1CF3778B62AC82519A97ACEBDC8D3DEE9F4FF0A15202B36414C57626D78838E99A4AFBAC5D0DBE6F1FC07121D28333E49545F6A"},
};

//! A test RSA key and the PKCS #1 v1.5 type 1 block of 200 bytes of i * 3 + 1 that its private key encrypts.
constexpr std::string_view inZOIPublicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxJumMnt9/9sNT/IqgRsl\n"
    "ncxlbersYThtGRXyy77GuLkDQhb+bcRfyuOBSDk2zby2mZVBGnVeeqgO/l6l/07V\n"
    "Fk6twcvCWrfLg/jFg3qXjMHTA1eC08JviynzBSu/FDkQrhEjLdtBD0bX9NAvrSQc\n"
    "194AQITwLI19FYyBNx1ayBtBhNLyOK3x7EJg+TkhDfPWyO4I7leFWcH6pCpCCnNr\n"
    "3I32zf/KPHzFjgjwFoJuSDZapYbbUTDUOcqDfDcoJVLQ6Soa5lQd9bdxSuGRFqnj\n"
    "x+UVP2qK/BdPCsKQ+ayQ+w0XuC+GhVxOZ+IlXDhFhVJksPymWwQc7VmUOsd5ya5f\n"
    "HwIDAQAB\n"
    "-----END PUBLIC KEY-----\n";
constexpr std::string_view inZOISignedFooter =
    "7F0EE5A8950D23A7438C999819E2D99B8B02F27725BA7CDF7D3730D600FA8C0ED627D63303775F3B588040EF905C81889FD2E5BDAE549C09806416DD20ED900C1F0E81D96A9348EE3CB9FB6870BF7CC00D1024783BBC0AB627EA90964CC2EAAF7491FFA6FC9AED25EC64BBA60106C7A94F35F762AEFC3D9C2E43B01CE86F19963B75E1309F2D4E7125AF9D3817A1B7CA9ED6582F93361B3F766C6FF1A0FF5C950779D510612C9DB9222A3FBA6E78CABBD5C9218787CFB8B56874ABF53CD3876A62DBB80D6CA69A576F0E0E6C29EB01E440A30A545EC89B7CBE24590F0D580A51B5D1B22CEC47DA9AE46ED26A9B8D6971D3EAFE417A49D3519F5FE31BB5E80F0E";

}

TEST(arenaBreakoutInfinitePackageSummaryMatchesCue4Parse) {
    for (const PackageVector& vector : packageVectors) {
        std::vector<std::byte> file = pattern(80, 11, 5);
        writeLittleEndian(file, 0, vector.magic, 4);
        writeLittleEndian(file, 6, 32, 2);
        CHECK(ue5parse::getArenaBreakoutInfiniteSummarySize(std::span<const std::byte, 8>(file.data(), 8)) == 40);
        REQUIRE(ue5parse::decryptArenaBreakoutInfinitePackage(file).has_value());
        CHECK(file == fromHex(vector.expected));
    }
}

TEST(arenaBreakoutInfiniteIniMatchesCue4Parse) {
    std::vector<std::byte> file = pattern(56, 5, 9);
    file[0] = std::byte{0x1B};
    file[2] = std::byte{0x55};
    file[3] = std::byte{0x41};
    writeLittleEndian(file, 4, 37, 4);
    CHECK(ue5parse::getArenaBreakoutInfiniteIniSize(std::span<const std::byte, 8>(file.data(), 8), file.size()) == 37);
    auto decrypted = ue5parse::decryptArenaBreakoutInfiniteIni(file);
    REQUIRE(decrypted.has_value());
    CHECK(*decrypted == fromHex("4AEB15D6C42ACA714746A7A161F6A8B1E7944CF5967DBB33D738A207D9DC8C3954BD4D8C7D"));
}

TEST(arenaBreakoutInfiniteLuaMatchesCue4Parse) {
    std::vector<std::byte> file = fromHex("1B55414D0104070A0D101316191C1F2225282B2E3134373A3D404346494C4F52808080112233826D7D422025726501816B070C11161B20252A808080808080");
    REQUIRE(ue5parse::decryptArenaBreakoutInfiniteLua(file).has_value());
    CHECK(file == fromHex("1B4C7561546C19930D0A1A0A717477785600000000000000000000002877403A808080794A5B824C1234565201020381036F6366706E4F4A60808080808080"));
}

TEST(arenaBreakoutInfiniteFileKindFollowsExtension) {
    CHECK(ue5parse::getArenaBreakoutInfiniteFile("Game/Config/DefaultGame.INI") == ArenaBreakoutInfiniteFile::Ini);
    CHECK(ue5parse::getArenaBreakoutInfiniteFile("Game/Script/Main.lua") == ArenaBreakoutInfiniteFile::Lua);
    CHECK(ue5parse::getArenaBreakoutInfiniteFile("Game/Maps/Level.umap") == ArenaBreakoutInfiniteFile::Package);
    CHECK(ue5parse::getArenaBreakoutInfiniteFile("Game/Mesh.uexp") == ArenaBreakoutInfiniteFile::Other);
}

TEST(inZOIPakInfoDecryptsSignedFooter) {
    std::vector<std::byte> tail(300, std::byte{0xAA});
    for (std::size_t i = 0; i < 7; i++)
        tail[37 + i] = static_cast<std::byte>(0x10 + i);
    std::vector<std::byte> signature = fromHex(inZOISignedFooter);
    std::memcpy(tail.data() + 44, signature.data(), signature.size());
    REQUIRE(ue5parse::decryptInZOIPakInfo(tail, std::as_bytes(std::span(inZOIPublicKey))).has_value());
    for (std::size_t i = 0; i < 37; i++)
        CHECK(tail[i] == std::byte{0xAA});
    CHECK(std::memcmp(tail.data() + 44, signature.data(), 49) == 0);
    for (std::size_t i = 0; i < 7; i++)
        CHECK(tail[93 + i] == static_cast<std::byte>(0x10 + i));
    for (std::size_t i = 0; i < 200; i++)
        CHECK(tail[100 + i] == static_cast<std::byte>(i * 3 + 1));
}

TEST(inZOIPakInfoNeedsPublicKey) {
    std::vector<std::byte> tail(300, std::byte{0x01});
    CHECK(!ue5parse::decryptInZOIPakInfo(tail, {}).has_value());
    std::vector<std::byte> clear(300, std::byte{0});
    CHECK(ue5parse::decryptInZOIPakInfo(clear, {}).has_value());
}

TEST(finalFantasy7RebirthIndexBufferUnpacksBatches) {
    std::vector<std::byte> data(28);
    appendWords(data, {1, 3, 0, 2, 0});
    appendWords(data, {4, 10, 11, 12, 13});
    appendWords(data, {2, 0 | (1 << 10) | (2 << 20), 1 | (2 << 10) | (3 << 20)});
    appendWords(data, {0, 0, 0, 0, 0});
    appendWords(data, {1, 0});
    appendWords(data, {1, 0, 0, 0, 0, 0, 0, 1, 1});
    data.resize(data.size() + 96 - 32);
    auto decoded = ue5parse::decodeFinalFantasy7RebirthIndexBuffer(data);
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->has_value());
    CHECK((*decoded)->sectionTriangleCounts == std::vector<std::uint32_t>{1});
    CHECK((*decoded)->indices == (std::vector<std::uint32_t>{11, 12, 13}));
}

TEST(finalFantasy7RebirthIndexBufferWithoutBatchesKeepsStockBuffer) {
    std::vector<std::byte> data(28);
    appendWords(data, {0});
    auto decoded = ue5parse::decodeFinalFantasy7RebirthIndexBuffer(data);
    REQUIRE(decoded.has_value());
    CHECK(!decoded->has_value());
}

namespace {

namespace rt = ue5parse::runtime;

class NamePatchArchive : public rt::FMemoryReaderView {
public:
    using FMemoryReaderView::FMemoryReaderView;
    using FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = Text.ToUtf8() == "RowName" ? rt::FName("RowName") : rt::FName();
        return *this;
    }
};

void appendFrozenPointer(std::vector<std::byte>& bytes, std::uint32_t target) {
    const std::uint64_t packed = (static_cast<std::uint64_t>(target - bytes.size()) << 1) | 1;
    appendWords(bytes, {static_cast<std::uint32_t>(packed), static_cast<std::uint32_t>(packed >> 32)});
}

}

TEST(finalFantasy7RebirthFrozenReaderFollowsPointersAndNames) {
    std::vector<std::byte> frozen;
    appendFrozenPointer(frozen, 80);
    appendWords(frozen, {2, 2});
    appendFrozenPointer(frozen, 88);
    appendWords(frozen, {3, 3});
    appendWords(frozen, {0, 0});
    appendFrozenPointer(frozen, 96);
    appendWords(frozen, {2, 2});
    appendFrozenPointer(frozen, 112);
    appendWords(frozen, {2, 32, 0xFFFFFFFF, 0});
    appendWords(frozen, {7, 9});
    appendWords(frozen, {0x00690048, 0, 0x11, 0x22, 0x33, 0x44});
    appendWords(frozen, {1});
    REQUIRE(frozen.size() == 116);
    std::vector<std::byte> patches;
    appendWords(patches, {0, 0, 1, 8});
    for (char c : std::string_view("RowName\0", 8))
        patches.push_back(static_cast<std::byte>(c));
    appendWords(patches, {1, 32});
    NamePatchArchive archive(patches);
    archive.SetGame(ue5parse::EGame::GAME_UE4_26);
    rt::FMemoryImageObject image = rt::FMemoryImageResult::LoadPatchesFromArchive(archive, frozen);
    REQUIRE(!archive.IsError());
    rt::FMemoryMappedImageReader reader(image);
    rt::TArray<std::int32_t> values = reader.ReadRawArray<std::int32_t>();
    REQUIRE(values.Num() == 2);
    CHECK(values[0] == 7);
    CHECK(values[1] == 9);
    CHECK(reader.ReadString().ToUtf8() == "Hi");
    CHECK(reader.ReadName() == rt::FName("RowName"));
    CHECK(reader.Tell() == 40);
    rt::TArray<std::int32_t> sparse = reader.ReadSparseArray<std::int32_t>([&reader] { return reader.Read<std::int32_t>(); }, 8);
    REQUIRE(sparse.Num() == 1);
    CHECK(sparse[0] == 0x11);
    CHECK(reader.Tell() == 80);
    CHECK(!reader.IsError());
}

TEST(finalFantasy7RebirthFrozenReaderRejectsMismatchedStringSize) {
    std::vector<std::byte> frozen;
    appendFrozenPointer(frozen, 16);
    appendWords(frozen, {3, 4});
    appendWords(frozen, {0x00690048, 0});
    std::vector<std::byte> patches;
    appendWords(patches, {0, 0, 0});
    NamePatchArchive patchArchive(patches);
    patchArchive.SetGame(ue5parse::EGame::GAME_UE4_26);
    rt::FMemoryImageObject image = rt::FMemoryImageResult::LoadPatchesFromArchive(patchArchive, frozen);
    rt::FMemoryMappedImageReader reader(image);
    CHECK(reader.ReadString().ToUtf8().empty());
    CHECK(reader.IsError());
}

TEST(propertyTypeGuessesAreGatedByGame) {
    using ue5parse::EGame;
    std::optional<ue5parse::GuessedMapTypes> map = ue5parse::guessMapPropertyTypes(EGame::GAME_DaysGone, "ScalarMap");
    REQUIRE(map.has_value());
    CHECK(map->keyType == "NameProperty");
    CHECK(map->valueType == "IntProperty");
    CHECK(!ue5parse::guessMapPropertyTypes(EGame::GAME_UE4_20, "ScalarMap").has_value());
    map = ue5parse::guessMapPropertyTypes(EGame::GAME_StateOfDecay2, "ComponentMap");
    REQUIRE(map.has_value());
    CHECK(map->keyType == "UInt32Property");
    CHECK(ue5parse::guessSetInnerType(EGame::GAME_HonorofKingsWorld, "LogicState") == "EnumProperty");
    CHECK(ue5parse::guessSetInnerType(EGame::GAME_UE5_1, "LogicState").empty());
    CHECK(ue5parse::guessSetStructType(EGame::GAME_Avowed, "QuestIDs") == "Guid");
    CHECK(ue5parse::guessSetStructType(EGame::GAME_Farlight84, "Anything") == "SoftObjectPath");
    CHECK(ue5parse::guessSetStructType(EGame::GAME_UE4_27, "AnimSequenceInstances") == "Guid");
    CHECK(ue5parse::guessSetStructType(EGame::GAME_DuneAwakening, "AnimSequenceInstances").empty());
    CHECK(ue5parse::guessSetStructType(EGame::GAME_DuneAwakening, "m_Placeables") == "StringEnumValue");
    CHECK(ue5parse::guessArrayStructType(EGame::GAME_DaysGone, "VertexData", 8) == "Vector2D");
    CHECK(ue5parse::guessArrayStructType(EGame::GAME_DaysGone, "VertexData", 12) == "Vector");
    CHECK(ue5parse::guessArrayStructType(EGame::GAME_DaysGone, "Keys", 27) == "RichCurveKey");
    CHECK(ue5parse::guessArrayStructType(EGame::GAME_DaysGone, "Keys", 28).empty());
    CHECK(ue5parse::guessArrayStructType(EGame::GAME_UE4_11, "Colors", 16).empty());
}

namespace {

std::uint32_t fnv1a(std::span<const std::byte> bytes) {
    std::uint32_t hash = 2166136261u;
    for (std::byte value : bytes)
        hash = (hash ^ static_cast<std::uint32_t>(value)) * 16777619u;
    return hash;
}

}

//! The checksums come from a Python port of CUE4Parse ACE7Decrypt and ACE7XORKey.
TEST(aceCombat7DecryptsHeaderAndExportsAsOneStream) {
    std::vector<std::byte> key(ue5parse::aceCombat7KeySize);
    for (std::size_t i = 0; i < key.size(); i++)
        key[i] = static_cast<std::byte>((i * 7 + 3) ^ (i >> 10));
    const ue5parse::AceCombat7Stream stream = ue5parse::getAceCombat7Stream("SK_TestAsset_Long_Name_For_Overflow_Check");
    CHECK(stream.row == 67);
    CHECK(stream.column == 858);
    CHECK(ue5parse::getAceCombat7FileName("Game/Content/Meshes/SK_Test.uasset") == "SK_Test");
    std::vector<std::byte> header = pattern(300, 13, 1);
    std::vector<std::byte> exports = pattern(200, 5, 9);
    ue5parse::decryptAceCombat7Header(std::span(header).first(2), 0, stream, key);
    ue5parse::decryptAceCombat7Header(std::span(header).subspan(2, 100), 2, stream, key);
    ue5parse::decryptAceCombat7Header(std::span(header).subspan(102), 102, stream, key);
    ue5parse::decryptAceCombat7Exports(std::span(exports).first(198), 0, header.size(), exports.size(), stream, key);
    ue5parse::decryptAceCombat7Exports(std::span(exports).subspan(198), 198, header.size(), exports.size(), stream, key);
    CHECK(fnv1a(header) == 0xda4aa0c9u);
    CHECK(fnv1a(exports) == 0x5473b5e2u);
}

TEST(darkPicturesMorphTargetDeltasApplyHalfScales) {
    std::vector<std::byte> data;
    appendWords(data, {1, 2});
    data.push_back(std::byte{3});
    data.push_back(std::byte{5});
    appendWords(data, {2});
    appendHalves(data, {0x3C00, 0x4000, 0x3800, 0x4000, 0x3C00, 0, 0, 0});
    appendHalves(data, {0x3800, 0x3800, 0x3800, 0x3800, 0x4000, 0x4000, 0x4000, 0x3C00});
    appendWords(data, {100});
    data.push_back(std::byte{0xFF});
    auto decoded = ue5parse::decodeDarkPicturesMorphTargetDeltas(data, true);
    REQUIRE(decoded.has_value());
    CHECK(decoded->bytesRead == data.size() - 1);
    REQUIRE(decoded->deltas.size() == 2);
    CHECK(deltaEquals(decoded->deltas[0], {2, 4, 1}, {1, 0, 0}, 103));
    CHECK(deltaEquals(decoded->deltas[1], {0.25f, 0.25f, 0.25f}, {2, 2, 2}, 105));
}

TEST(darkPicturesMorphTargetDeltasOfLittleHopeHaveNoScale) {
    std::vector<std::byte> data;
    appendWords(data, {1, 1});
    data.push_back(std::byte{7});
    appendWords(data, {1});
    appendHalves(data, {0x3C00, 0xC000, 0x0001, 0x3800, 0, 0xBC00});
    appendWords(data, {10});
    auto decoded = ue5parse::decodeDarkPicturesMorphTargetDeltas(data, false);
    REQUIRE(decoded.has_value());
    CHECK(decoded->bytesRead == data.size());
    REQUIRE(decoded->deltas.size() == 1);
    CHECK(deltaEquals(decoded->deltas[0], {1, -2, 0x1p-24f}, {0.5f, 0, -1}, 17));
}

TEST(darkPicturesMorphTargetDeltasNeedAnIndexForEachVertex) {
    std::vector<std::byte> data;
    appendWords(data, {1, 0, 1});
    appendHalves(data, {0, 0, 0, 0, 0, 0});
    appendWords(data, {0});
    CHECK(!ue5parse::decodeDarkPicturesMorphTargetDeltas(data, false).has_value());
}

TEST(rocoKingdomWorldMorphTargetDeltaUnpacksBits) {
    const std::uint64_t packed = 0x7Full | (0x81ull << 8) | (0x3Full << 24) | (0x7ull << 38) | (12345ull << 45);
    CHECK(deltaEquals(ue5parse::decodeRocoKingdomWorldMorphTargetDelta(packed), {127 / 127.5f, -127 / 127.5f, 0}, {63 / 63.5f, 0, 7 / 63.5f}, 12345));
}

namespace {

void appendString(std::vector<std::byte>& bytes, std::string_view text) {
    appendWords(bytes, {static_cast<std::uint32_t>(text.size() + 1)});
    for (char c : text)
        bytes.push_back(static_cast<std::byte>(c));
    bytes.push_back(std::byte{0});
}

void appendData(std::vector<std::byte>& bytes, const std::vector<std::byte>& data) {
    appendWords(bytes, {static_cast<std::uint32_t>(data.size())});
    bytes.insert(bytes.end(), data.begin(), data.end());
}

ue5parse::MountedGame emptyOuterWorlds2() {
    ue5parse::MountOptions options;
    options.game = ue5parse::EGame::GAME_OuterWorlds2;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

template <class T>
const T* findOW2Value(const rt::FInstancedStruct& Value, const char* Name) {
    const rt::UScriptStruct* Struct = Value.GetScriptStruct();
    const rt::FProperty* Property = Struct == nullptr ? nullptr : Struct->FindPropertyByName(rt::FName(Name));
    return Property == nullptr ? nullptr : Property->ContainerPtrToValuePtr<T>(Value.GetMemory());
}

}

TEST(outerWorlds2PropertyDataReadsTaggedListWithNamesAsStrings) {
    ue5parse::FileProvider provider(emptyOuterWorlds2());
    rt::FRuntime runtime(provider);
    std::vector<std::byte> data;
    appendString(data, "Count");
    appendString(data, "IntProperty");
    appendWords(data, {4, 0});
    data.push_back(std::byte{0});
    appendWords(data, {7});
    appendString(data, "None");
    std::vector<std::byte> bytes;
    appendWords(bytes, {0});
    appendData(bytes, data);
    appendWords(bytes, {0, 0});
    rt::FMemoryReaderView Ar(bytes);
    Ar.SetGame(ue5parse::EGame::GAME_OuterWorlds2);
    rt::FPropertyDataWithObjects Value;
    CHECK(Value.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    const auto* Count = findOW2Value<std::int32_t>(Value.Properties, "Count");
    REQUIRE(Count != nullptr);
    CHECK(*Count == 7);
}

TEST(outerWorlds2PropertyTagsReadBoolsAndKnownArrays) {
    ue5parse::FileProvider provider(emptyOuterWorlds2());
    rt::FRuntime runtime(provider);
    std::vector<std::byte> enabled;
    appendWords(enabled, {0});
    enabled.push_back(std::byte{1});
    std::vector<std::byte> waves;
    appendWords(waves, {0});
    appendString(waves, "IntProperty");
    appendWords(waves, {12, 2, 3, 5});
    std::vector<std::byte> bytes;
    appendWords(bytes, {2});
    appendString(bytes, "Enabled");
    appendWords(bytes, {0});
    appendString(bytes, "BoolProperty");
    appendWords(bytes, {0});
    appendData(bytes, enabled);
    appendWords(bytes, {0, 0});
    appendString(bytes, "WaveIndexes");
    appendWords(bytes, {0});
    appendString(bytes, "ArrayProperty");
    appendWords(bytes, {0});
    appendData(bytes, waves);
    appendWords(bytes, {0, 0});
    rt::FMemoryReaderView Outer(bytes);
    Outer.SetGame(ue5parse::EGame::GAME_OuterWorlds2);
    rt::FPropertyDataObjectContainer Objects;
    rt::FOW2ObjectsArchive Ar(bytes, Outer, Objects, false);
    rt::FInstancedStruct Value;
    rt::SerializeOW2PropertyTags(Ar, Value);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    const auto* Enabled = findOW2Value<bool>(Value, "Enabled");
    REQUIRE(Enabled != nullptr);
    CHECK(*Enabled);
    const auto* Waves = findOW2Value<rt::TArray<std::int32_t>>(Value, "WaveIndexes");
    REQUIRE(Waves != nullptr);
    REQUIRE(Waves->Num() == 2);
    CHECK((*Waves)[0] == 3);
    CHECK((*Waves)[1] == 5);
}

TEST(subnautica2SpatialLayerReadsThreeByteCells) {
    std::vector<std::byte> bytes;
    appendWords(bytes, {1, 0x12345678, 0x9});
    bytes.push_back(std::byte{1});
    bytes.push_back(std::byte{0});
    bytes.push_back(std::byte{1});
    rt::FMemoryReaderView Ar(bytes);
    rt::FUWEWorldPopSpatialLayer Layer;
    Ar.SetGame(ue5parse::EGame::GAME_UE5_4);
    CHECK(!Layer.Serialize(Ar));
    Ar.SetGame(ue5parse::EGame::GAME_Subnautica2);
    CHECK(Layer.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    const rt::FUWEWorldPopSpatialCell* Cell = Layer.CellMap.Find(0x912345678ll);
    REQUIRE(Cell != nullptr);
    CHECK(Cell->IsEmpty);
    CHECK(!Cell->IsPlayableSpace);
    CHECK(Cell->IsLandscapeCell);
}
