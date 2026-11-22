#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CppUE5Parse/GameTypes/NevernessToEverness/NevernessToEvernessIni.h"
#include "CppUE5Parse/GameTypes/Snowbreak/SnowbreakLua.h"
#include "CppUE5Parse/GameTypes/Strinova/StrinovaLua.h"
#include "CppUE5Parse/IO/FileSource.h"
#include "CppUE5Parse/VFS/FileSystem.h"
#include "Test.h"

namespace {

using ue5parse::EGame;

class MemorySource final : public ue5parse::FileSource {
public:
    MemorySource(std::string name, std::vector<std::byte> bytes) : name(std::move(name)), bytes(std::move(bytes)) {}

    std::string_view getName() const noexcept override { return name; }
    std::uint64_t getSize() const noexcept override { return bytes.size(); }
    ue5parse::Result<void> read(std::uint64_t offset, std::span<std::byte> buffer) const noexcept override {
        if (offset > bytes.size() || buffer.size() > bytes.size() - offset)
            return ue5parse::fail(ue5parse::ErrorCode::ReadPastEnd, "MemorySource.read", offset);
        std::memcpy(buffer.data(), bytes.data() + offset, buffer.size());
        return {};
    }

private:
    std::string name;
    std::vector<std::byte> bytes;
};

class Bytes {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = data.size();
        data.resize(at + sizeof(T));
        std::memcpy(data.data() + at, &value, sizeof(T));
    }

    void fill(std::size_t count, std::uint8_t value) { data.resize(data.size() + count, static_cast<std::byte>(value)); }

    void append(std::span<const std::byte> bytes) { data.insert(data.end(), bytes.begin(), bytes.end()); }

    void string(std::string_view text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        append(std::as_bytes(std::span(text)));
        put<std::uint8_t>(0);
    }

    std::vector<std::byte> data;
};

std::vector<std::byte> toBytes(std::string_view text) {
    auto bytes = std::as_bytes(std::span(text));
    return {bytes.begin(), bytes.end()};
}

//! An LZ4 block of one literal run.
std::vector<std::byte> makeLz4Literals(std::span<const std::byte> literals) {
    Bytes block;
    if (literals.size() < 15) {
        block.put<std::uint8_t>(static_cast<std::uint8_t>(literals.size() << 4));
    } else {
        block.put<std::uint8_t>(0xF0);
        block.put<std::uint8_t>(static_cast<std::uint8_t>(literals.size() - 15));
    }
    block.append(literals);
    return block.data;
}

struct GameForPeaceFile {
    std::uint64_t offset;
    std::uint64_t size;
    std::uint64_t uncompressedSize;
    std::uint32_t method;
    std::uint64_t blockStart;
};

void writeGameForPeaceEntry(Bytes& out, const GameForPeaceFile& file) {
    out.fill(20, 0xAA);
    out.put(file.offset);
    out.put(file.uncompressedSize);
    out.put(file.method);
    out.put(file.size);
    out.fill(21, 0);
    if (file.method != 0) {
        out.put<std::uint32_t>(1);
        out.put(file.blockStart);
        out.put(file.blockStart + file.size);
    }
    out.put<std::uint32_t>(file.method != 0 ? 0x10000 : 0);
    out.put<std::uint8_t>(0);
}

//! Writes a file with its entry header at the end of the pak data.
GameForPeaceFile addGameForPeaceFile(Bytes& pak, std::span<const std::byte> content, bool compressed) {
    std::vector<std::byte> stored = compressed ? makeLz4Literals(content) : std::vector<std::byte>(content.begin(), content.end());
    GameForPeaceFile file{pak.data.size(), stored.size(), content.size(), compressed ? 4u : 0u, 0};
    std::uint64_t headerSize = 20 + 8 + 8 + 4 + 8 + 21 + (compressed ? 4 + 16 : 0) + 4 + 1;
    file.blockStart = file.offset + headerSize;
    writeGameForPeaceEntry(pak, file);
    pak.append(stored);
    return file;
}

void writeGameForPeaceFooter(Bytes& pak, std::uint64_t indexOffset, std::uint64_t indexSize) {
    pak.put<std::uint8_t>(0x6C);
    pak.put<std::uint32_t>(0xFF67FF70);
    pak.put<std::int32_t>(11);
    pak.fill(20, 0);
    pak.put<std::uint64_t>(indexSize ^ 0x8924B0E3298B7069);
    pak.put<std::uint64_t>(indexOffset ^ 0xD74AF37FAA6B020D);
}

std::map<std::string, std::vector<std::byte>> readAllFiles(std::vector<std::byte> pak, EGame game) {
    std::map<std::string, std::vector<std::byte>> out;
    std::vector<std::unique_ptr<ue5parse::FileSource>> sources;
    sources.push_back(std::make_unique<MemorySource>("Test_P.pak", std::move(pak)));
    auto set = ue5parse::readArchives(std::move(sources), nullptr, nullptr, game);
    if (!set || set->archives.size() != 1)
        return out;
    ue5parse::FileSystem system(std::move(*set));
    std::array<char, 256> buffer{};
    for (const ue5parse::FileEntry& file : system.getFiles()) {
        std::vector<std::byte> content(static_cast<std::size_t>(system.getFileSize(file)));
        if (!system.readFile(file, 0, content))
            continue;
        out.emplace(std::string(system.writePath(file, buffer)), std::move(content));
    }
    return out;
}

constexpr std::array<std::uint8_t, 8> gameForPeaceIniStart = {0x7D, 0x5B, 0x5D, 0x5D, 0x58, 0x57, 0x44, 0x4B};
constexpr std::string_view gameForPeaceIniKey = "123456789234567891345678912456789123567891234678912345789123456891234567912345678";

std::vector<std::byte> makeGameForPeaceIni(std::size_t size) {
    std::vector<std::byte> stored(size);
    for (std::size_t i = 0; i < size; i++)
        stored[i] = static_cast<std::byte>(i < gameForPeaceIniStart.size() ? gameForPeaceIniStart[i] : 0x40 + i);
    return stored;
}

bool isGameForPeaceIniPlain(std::span<const std::byte> stored, std::span<const std::byte> read) {
    if (stored.size() != read.size())
        return false;
    for (std::size_t i = 0; i < stored.size(); i++)
        if ((stored[i] ^ static_cast<std::byte>(gameForPeaceIniKey[i % gameForPeaceIniKey.size()])) != read[i])
            return false;
    return true;
}

//! An uncompressed file with a zero entry header, and its encoded entry with 32-bit fields.
void addEncodedFile(Bytes& pak, Bytes& encoded, std::string_view content) {
    std::uint32_t offset = static_cast<std::uint32_t>(pak.data.size());
    pak.fill(53, 0);
    pak.append(std::as_bytes(std::span(content)));
    encoded.put<std::uint32_t>((1u << 31) | (1u << 30) | (1u << 29));
    encoded.put(offset);
    encoded.put(static_cast<std::uint32_t>(content.size()));
}

void writeStandardFooter(Bytes& pak, std::uint64_t indexOffset, std::uint64_t indexSize) {
    pak.fill(16, 0);
    pak.put<std::uint8_t>(0);
    pak.put<std::uint32_t>(0x5A6F12E1);
    pak.put<std::int32_t>(11);
    pak.put(indexOffset);
    pak.put(indexSize);
    pak.fill(20 + 32 * 5, 0);
}

void putXorString(Bytes& out, std::string_view text, std::uint8_t key) {
    out.put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
    for (char c : text)
        out.put<std::uint8_t>(static_cast<std::uint8_t>(c) ^ key);
    out.put<std::uint8_t>(key);
}

}

TEST(crystalOfAtlanNamesHashedFilesAfterDirectory) {
    Bytes pak;
    Bytes encoded;
    addEncodedFile(pak, encoded, "alpha");
    addEncodedFile(pak, encoded, "bravo");
    Bytes directory;
    directory.put<std::int32_t>(1);
    directory.string("/");
    directory.put<std::int32_t>(1);
    directory.string("a.txt");
    directory.put<std::int32_t>(0);
    directory.put<std::int32_t>(2);
    directory.put<std::uint64_t>(111);
    directory.put<std::int32_t>(0);
    directory.put<std::uint64_t>(222);
    directory.put<std::int32_t>(12);
    std::uint64_t directoryOffset = pak.data.size();
    pak.append(directory.data);
    std::uint64_t indexOffset = pak.data.size();
    pak.put<std::int32_t>(2);
    pak.string("../../../Seria/Content/");
    pak.fill(3, 0);
    pak.put<std::uint64_t>(0);
    pak.put<std::int32_t>(static_cast<std::int32_t>(encoded.data.size()));
    pak.append(encoded.data);
    pak.fill(8, 0);
    pak.put<std::uint32_t>(1);
    pak.put(directoryOffset);
    pak.put<std::uint64_t>(directory.data.size());
    std::uint64_t indexSize = pak.data.size() - indexOffset;
    pak.put<std::uint8_t>(0);
    pak.put<std::int32_t>(11);
    pak.put(indexSize);
    pak.fill(20, 0);
    pak.put(indexOffset);
    pak.put<std::uint32_t>(0x22CE976A);
    pak.fill(16 + 32 * 5, 0);

    auto files = readAllFiles(std::move(pak.data), EGame::GAME_CrystalOfAtlan);
    REQUIRE(files.size() == 2);
    CHECK(files["Seria/Content/a.txt"] == toBytes("alpha"));
    CHECK(files["Seria/Content/222"] == toBytes("bravo"));
}

TEST(dragonSwordDecodesIndexStrings) {
    Bytes pak;
    Bytes encoded;
    addEncodedFile(pak, encoded, "delta");
    constexpr std::uint8_t entryKey = 0x37;
    for (std::byte& value : encoded.data)
        value ^= std::byte{entryKey};
    REQUIRE(encoded.data[5] == std::byte{entryKey});
    Bytes directory;
    directory.put<std::int32_t>(1);
    putXorString(directory, "/Maps/", 0x21);
    directory.put<std::int32_t>(1);
    putXorString(directory, "d.txt", 0x42);
    directory.put<std::int32_t>(0);
    std::uint64_t directoryOffset = pak.data.size();
    pak.append(directory.data);
    std::uint64_t indexOffset = pak.data.size();
    putXorString(pak, "../../../Game/", 0x5A);
    pak.put<std::int32_t>(1);
    pak.put<std::uint64_t>(0);
    pak.put<std::uint32_t>(1);
    pak.fill(36, 0);
    pak.put<std::uint32_t>(1);
    pak.put(directoryOffset);
    pak.put<std::uint64_t>(directory.data.size());
    pak.fill(20, 0);
    pak.put<std::int32_t>(static_cast<std::int32_t>(encoded.data.size()));
    pak.append(encoded.data);
    pak.put<std::int32_t>(0);
    writeStandardFooter(pak, indexOffset, pak.data.size() - indexOffset);

    auto files = readAllFiles(std::move(pak.data), EGame::GAME_DragonSwordAwakening);
    REQUIRE(files.size() == 1);
    CHECK(files["Game/Maps/d.txt"] == toBytes("delta"));
}

TEST(gameForPeaceOldIndexReadsDirectoryAfterIndex) {
    Bytes pak;
    std::vector<std::byte> ini = makeGameForPeaceIni(100);
    std::vector<std::byte> text = toBytes("plain text");
    GameForPeaceFile iniFile = addGameForPeaceFile(pak, ini, true);
    GameForPeaceFile textFile = addGameForPeaceFile(pak, text, false);
    GameForPeaceFile copyFile = addGameForPeaceFile(pak, ini, false);
    std::uint64_t indexOffset = pak.data.size();
    pak.string("../../../Game/");
    pak.put<std::int32_t>(3);
    writeGameForPeaceEntry(pak, iniFile);
    writeGameForPeaceEntry(pak, textFile);
    writeGameForPeaceEntry(pak, copyFile);
    std::uint64_t indexSize = pak.data.size() - indexOffset;
    Bytes directory;
    directory.put<std::int64_t>(2);
    directory.string("/");
    directory.put<std::int64_t>(1);
    directory.string("Config.ini");
    directory.put<std::int32_t>(0);
    directory.string("/Data/");
    directory.put<std::int64_t>(2);
    directory.string("Notes.txt");
    directory.put<std::int32_t>(1);
    directory.string("Stored.ini");
    directory.put<std::int32_t>(2);
    pak.put<std::int64_t>(static_cast<std::int64_t>(directory.data.size()));
    pak.append(directory.data);
    writeGameForPeaceFooter(pak, indexOffset, indexSize);

    auto files = readAllFiles(std::move(pak.data), EGame::GAME_GameForPeace);
    REQUIRE(files.size() == 3);
    REQUIRE(files.contains("Game/Config.ini"));
    REQUIRE(files.contains("Game/Data/Notes.txt"));
    REQUIRE(files.contains("Game/Data/Stored.ini"));
    CHECK(isGameForPeaceIniPlain(ini, files["Game/Config.ini"]));
    CHECK(files["Game/Data/Notes.txt"] == text);
    CHECK(files["Game/Data/Stored.ini"] == ini);
}

TEST(gameForPeaceMapIndexReadsNamesBeforeEntries) {
    Bytes pak;
    std::vector<std::byte> ini = makeGameForPeaceIni(30);
    GameForPeaceFile iniFile = addGameForPeaceFile(pak, ini, true);
    std::uint64_t indexOffset = pak.data.size();
    pak.string("../../../Game/");
    pak.put<std::int32_t>(1);
    pak.string("Config/Default.ini");
    writeGameForPeaceEntry(pak, iniFile);
    writeGameForPeaceFooter(pak, indexOffset, pak.data.size() - indexOffset);

    auto files = readAllFiles(std::move(pak.data), EGame::GAME_GameForPeace);
    REQUIRE(files.contains("Game/Config/Default.ini"));
    CHECK(isGameForPeaceIniPlain(ini, files["Game/Config/Default.ini"]));
}

TEST(snowbreakLuaDecryptsFullBlocks) {
    constexpr std::uint8_t stored[] = {0xBC, 0x08, 0xD7, 0x48, 0xB4, 0x0B, 0xBA, 0xD3, 0x65, 0x83, 0x04, 0xDC, 0x16, 0x3E, 0xD0, 0xDC, 0x58, 0xB1, 0x82, 0x3F,
                                       0x7A, 0xE6, 0x72, 0xF7, 0x39, 0xE7, 0x82, 0xC2, 0x38, 0xF1, 0xD6, 0x5F, 0xA5, 0xB2, 0xBF, 0xCC, 0xD9, 0xE6, 0xF3, 0x00};
    std::vector<std::byte> file(sizeof(stored));
    std::memcpy(file.data(), stored, sizeof(stored));
    ue5parse::decryptSnowbreakLua(file);
    bool plain = true;
    for (std::size_t i = 0; i < file.size(); i++)
        plain = plain && file[i] == static_cast<std::byte>((i * 13 + 5) & 0xFF);
    CHECK(plain);
}

TEST(strinovaLuaRepeatsKey) {
    std::vector<std::byte> file(300);
    ue5parse::decryptStrinovaLua(file);
    CHECK(file[0] != std::byte{0});
    CHECK(file[256] == file[0]);
    CHECK(file[299] == file[43]);
    ue5parse::decryptStrinovaLua(file);
    CHECK(file[0] == std::byte{0});
}

TEST(nevernessToEvernessIniDecryptsLines) {
    std::string text = "\xEF\xBB\xBF[Section]\r\nv6woPXmFxN8/pPP5J2WLPKXwkxXeeP6bMbay8tZrYAk=\r\n\nplain=2\n";
    auto decrypted = ue5parse::decryptNevernessToEvernessIni(toBytes(text));
    REQUIRE(decrypted.has_value());
    CHECK(*decrypted == toBytes("[Section]\r\nKey=Value\r\nOther=1\r\nplain=2\r\n"));

    auto chinese = ue5parse::decryptNevernessToEvernessIni(toBytes("/0oOCXjH1ufQqG0sA6MyAg==\n"));
    REQUIRE(chinese.has_value());
    CHECK(*chinese == toBytes("Cn=1\r\n"));

    auto blank = ue5parse::decryptNevernessToEvernessIni(toBytes(" \r\n"));
    REQUIRE(blank.has_value());
    CHECK(*blank == toBytes(" \r\n"));

    CHECK(!ue5parse::decryptNevernessToEvernessIni(toBytes("AAAAAAAAAAAAAAAAAAAAAA==\n")).has_value());
}
