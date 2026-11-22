#include <cstdint>
#include <utility>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Crypto/Blake3.h"
#include "CppUE5Parse/VFS/Pak.h"
#include "Test.h"

namespace {

using ue5parse::decodePakEntry;
using ue5parse::EGame;
using ue5parse::EPakFileVersion;
using ue5parse::FPakInfo;
using ue5parse::PakEntries;
using ue5parse::parsePakInfo;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    void zeros(std::size_t count) { bytes.resize(bytes.size() + count); }

    void name(const char* text) {
        std::size_t at = bytes.size();
        zeros(FPakInfo::methodNameLength);
        std::memcpy(bytes.data() + at, text, std::strlen(text));
    }

    std::span<const std::byte> span() const { return bytes; }

    std::vector<std::byte> bytes;
};

void writeStandardFooter(Writer& footer, std::uint32_t magic, std::int32_t version, std::int64_t indexOffset, std::int64_t indexSize) {
    footer.zeros(16);
    footer.put<std::uint8_t>(0);
    footer.put(magic);
    footer.put(version);
    footer.put(indexOffset);
    footer.put(indexSize);
    footer.zeros(20);
}

}

TEST(snowbreakPakFooterOffset) {
    Writer footer;
    writeStandardFooter(footer, FPakInfo::magic, 11, 0x1000 ^ 0x1C1D1E1F, 0x80);
    footer.name("Oodle");
    footer.zeros(FPakInfo::methodNameLength * 4);
    auto info = parsePakInfo(footer.span(), EGame::GAME_Snowbreak);
    REQUIRE(info.has_value());
    CHECK(info->indexOffset == 0x1000);
    CHECK(info->indexSize == 0x80);
    CHECK(info->methodCount == 2);
    CHECK(info->getMethodName(1) == "Oodle");
}

TEST(dreamStarPakFooterSwapsIndexFields) {
    Writer footer;
    writeStandardFooter(footer, 0x1B6A32F1, 11, 0x40, 0x2000);
    footer.zeros(FPakInfo::methodNameLength * 5 + 10);
    auto info = parsePakInfo(footer.span(), EGame::GAME_DreamStar);
    REQUIRE(info.has_value());
    CHECK(info->indexOffset == 0x2000);
    CHECK(info->indexSize == 0x40);
}

TEST(gameForPeacePakFooterKeys) {
    Writer footer;
    footer.put<std::uint8_t>(0x6c);
    footer.put<std::uint32_t>(0xff67ff70);
    footer.put<std::int32_t>(11);
    footer.zeros(20);
    footer.put<std::uint64_t>(0x300 ^ 0x8924b0e3298b7069);
    footer.put<std::uint64_t>(0x5000 ^ 0xd74af37faa6b020d);
    auto info = parsePakInfo(footer.span(), EGame::GAME_GameForPeace);
    REQUIRE(info.has_value());
    CHECK(!info->encryptedIndex);
    CHECK(info->version == EPakFileVersion::FNameBasedCompressionMethod);
    CHECK(info->indexOffset == 0x5000);
    CHECK(info->indexSize == 0x300);
    CHECK(info->methodCount == 6);
    CHECK(info->getMethodName(4) == "LZ4");
}

TEST(promiseMascotAgencyPakFooterVersion) {
    Writer footer;
    footer.zeros(16);
    footer.put<std::uint8_t>(0);
    footer.put<std::uint32_t>(0x11adde11);
    footer.zeros(20);
    footer.put<std::int32_t>(0 ^ 0x0A4FFC11);
    footer.zeros(8);
    footer.put<std::int64_t>(0x700 ^ 0x0BBEFB6F91D3B57B);
    footer.put<std::int64_t>(0x9000);
    footer.zeros(229 - footer.bytes.size());
    auto info = parsePakInfo(footer.span(), EGame::GAME_PromiseMascotAgency);
    REQUIRE(info.has_value());
    CHECK(info->version == EPakFileVersion::Fnv64BugFix);
    CHECK(info->indexOffset == 0x9000);
    CHECK(info->indexSize == 0x700);
}

TEST(customGameRejectsWrongMagic) {
    Writer footer;
    writeStandardFooter(footer, 0x12345678, 11, 0x40, 0x40);
    footer.zeros(FPakInfo::methodNameLength * 5);
    CHECK(!parsePakInfo(footer.span(), EGame::GAME_Snowbreak).has_value());
}

TEST(wutheringWavesEncodedEntry) {
    FPakInfo info;
    info.game = EGame::GAME_WutheringWaves;
    info.version = EPakFileVersion::Utf8PakDirectory;
    //! Engine bits: 32-bit offset, raw size and size, one block of 64 KiB, no method; Wuthering Waves moves them.
    std::uint32_t engine = (1u << 31) | (1u << 30) | (1u << 29) | (1u << 6) | 32;
    std::uint32_t stored = ((engine >> 6) & 0xFFFF) | ((engine & 0x3F) << 16) | ((engine & (1u << 22)) << 6) | ((engine >> 1) & 0x0FC00000)
                           | ((engine & 0x80000000) >> 2) | ((engine & 0x60000000) << 1);
    Writer encoded;
    encoded.put(stored);
    encoded.put<std::uint8_t>(3);
    encoded.put<std::uint32_t>(0x500);
    encoded.put<std::uint32_t>(0x2000);
    PakEntries entries;
    auto entry = decodePakEntry(encoded.span(), 0, info, entries);
    REQUIRE(entry.has_value());
    CHECK(entries.entries[*entry].offset == 0x2000);
    CHECK(entries.entries[*entry].uncompressedSize == 0x500);
    CHECK(entries.entries[*entry].customData == 3);
    CHECK(entries.entries[*entry].encoded);
}

TEST(blackMythWukongEncodedHeaderPadding) {
    FPakInfo info;
    info.game = EGame::GAME_BlackMythWukong;
    info.version = EPakFileVersion::Fnv64BugFix;
    Writer encoded;
    encoded.put<std::uint32_t>((1u << 31) | (1u << 30));
    encoded.put<std::uint32_t>(0x100);
    encoded.put<std::uint32_t>(0x10);
    PakEntries entries;
    auto entry = decodePakEntry(encoded.span(), 0, info, entries);
    REQUIRE(entry.has_value());
    CHECK(entries.entries[*entry].getHeaderSize(info) == 54);
}

namespace {

struct Blake3Vector {
    std::size_t length;
    std::string_view digest;
};

//! Digests from the BLAKE3 package that CUE4Parse uses, for the input bytes i % 251.
constexpr Blake3Vector blake3Vectors[] = {
    {0, "AF1349B9F5F9A1A6A0404DEA36DCC9499BCB25C9ADC112B7CC9A93CAE41F3262"},
    {1, "2D3ADEDFF11B61F14C886E35AFA036736DCD87A74D27B5C1510225D0F592E213"},
    {63, "E9BC37A594DAAD83BE9470DF7F7B3798297C3D834CE80BA85D6E207627B7DB7B"},
    {64, "4EED7141EA4A5CD4B788606BD23F46E212AF9CACEBACDC7D1F4C6DC7F2511B98"},
    {65, "DE1E5FA0BE70DF6D2BE8FFFD0E99CEAA8EB6E8C93A63F2D8D1C30ECB6B263DEE"},
    {1023, "10108970EEDA3EB932BAAC1428C7A2163B0E924C9A9E25B35BBA72B28F70BD11"},
    {1024, "42214739F095A406F3FC83DEB889744AC00DF831C10DAA55189B5D121C855AF7"},
    {1025, "D00278AE47EB27B34FAECF67B4FE263F82D5412916C1FFD97C8CB7FB814B8444"},
    {2048, "E776B6028C7CD22A4D0BA182A8BF62205D2EF576467E838ED6F2529B85FBA24A"},
    {2049, "5F4D72F40D7A5F82B15CA2B2E44B1DE3C2EF86C426C95C1AF0B6879522563030"},
    {3072, "B98CB0FF3623BE03326B373DE6B9095218513E64F1EE2EDD2525C7AD1E5CFFD2"},
    {3073, "7124B49501012F81CC7F11CA069EC9226CECB8A2C850CFE644E327D22D3E1CD3"},
    {4097, "9B4052B38F1C5FC8B1F9FF7AC7B27CD242487B3D890D15C96A1C25B8AA0FB995"},
    {8193, "BAB6C09CB8CE8CF459261398D2E7AEF35700BF488116CEB94A36D0F5F1B7BC3B"},
    {31744, "62B6960E1A44BCC1EB1A611A8D6235B6B4B78F32E7ABC4FB4C6CDCCE94895C47"},
};

std::string toHex(std::span<const std::uint8_t> data) {
    constexpr char digits[] = "0123456789ABCDEF";
    std::string text;
    for (std::uint8_t value : data) {
        text.push_back(digits[value >> 4]);
        text.push_back(digits[value & 0xF]);
    }
    return text;
}

}

TEST(blake3MatchesReference) {
    for (const Blake3Vector& vector : blake3Vectors) {
        std::vector<std::byte> input(vector.length);
        for (std::size_t i = 0; i < input.size(); i++)
            input[i] = static_cast<std::byte>(i % 251);
        CHECK(toHex(ue5parse::hashBlake3(input)) == vector.digest);
    }
}

TEST(marvelRivalsEncryptedLimitHashesPath) {
    FPakInfo info;
    info.game = EGame::GAME_MarvelRivals;
    ue5parse::FPakEntry first;
    ue5parse::setEncryptedLimit(info, "Marvel/Content/Marvel/Characters/1011/Meshes/SK_1011.uasset", first);
    CHECK(first.encryptedLimit == 1536);
    ue5parse::FPakEntry second;
    ue5parse::setEncryptedLimit(info, "Marvel/Content/Paks/Test.umap", second);
    CHECK(second.encryptedLimit == 3840);
}

TEST(wutheringWavesEncryptedLimitFollowsEntryByte) {
    FPakInfo info;
    info.game = EGame::GAME_WutheringWaves;
    constexpr std::pair<std::uint8_t, std::uint32_t> limits[] = {{0, UINT32_MAX}, {1, 0x200000}, {2, 0x800}, {4, 0}};
    for (auto [data, limit] : limits) {
        ue5parse::FPakEntry entry;
        entry.customData = data;
        ue5parse::setEncryptedLimit(info, "Client/Content/a.uasset", entry);
        CHECK(entry.encryptedLimit == limit);
    }
}
