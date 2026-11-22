#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/GameTypes/ArenaBreakoutInfinite/ArenaBreakoutInfiniteAes.h"
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
