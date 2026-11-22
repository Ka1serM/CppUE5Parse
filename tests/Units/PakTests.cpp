#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

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
