#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Package/LegacyPackage.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::parseLegacyPackageHeader;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    void zeros(std::size_t count) { bytes.resize(bytes.size() + count); }

    void fill(std::size_t count) { bytes.resize(bytes.size() + count, std::byte{0xFF}); }

    void string(std::string_view text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        for (char c : text)
            put(c);
        put('\0');
    }

    template <typename T>
    void patch(std::size_t at, T value) {
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    std::vector<std::byte> bytes;
};

struct SummaryFields {
    std::uint32_t tag = 0x9E2A83C1;
    std::uint32_t mask = 0;
    std::int32_t licensee = 0;
    std::size_t extraAfterGuid = 0;
    std::size_t extraBeforeBulkData = 0;
};

//! A UE 4.26 cooked summary with one name, one import and no exports.
std::vector<std::byte> writePackage(const SummaryFields& fields) {
    Writer out;
    out.put(fields.tag);
    out.put<std::int32_t>(-7);
    out.put<std::int32_t>(864);
    out.put<std::int32_t>(522);
    out.put(fields.licensee);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    out.string("None");
    out.put<std::uint32_t>(0x80000000);
    std::size_t nameCountAt = out.bytes.size();
    out.zeros(8);
    out.zeros(8);
    std::size_t exportAt = out.bytes.size();
    out.zeros(16);
    out.zeros(4 + 8 + 4 + 4);
    out.zeros(16);
    out.fill(fields.extraAfterGuid);
    out.put<std::int32_t>(0);
    for (int i = 0; i < 2; i++) {
        out.zeros(10);
        out.string("");
    }
    out.zeros(4);
    out.put<std::int32_t>(0);
    out.zeros(4);
    out.put<std::int32_t>(0);
    out.zeros(4);
    out.fill(fields.extraBeforeBulkData);
    out.put<std::int64_t>(0);
    out.zeros(4);
    out.put<std::int32_t>(0);
    out.zeros(8);
    std::size_t nameOffset = out.bytes.size();
    out.string("None");
    out.zeros(4);
    std::size_t importOffset = out.bytes.size();
    out.zeros(28);
    out.patch<std::uint32_t>(nameCountAt, 1 ^ fields.mask);
    out.patch<std::uint32_t>(nameCountAt + 4, static_cast<std::uint32_t>(nameOffset) ^ fields.mask);
    out.patch<std::uint32_t>(exportAt, 0 ^ fields.mask);
    out.patch<std::uint32_t>(exportAt + 4, 0 ^ fields.mask);
    out.patch<std::uint32_t>(exportAt + 8, 1 ^ fields.mask);
    out.patch<std::uint32_t>(exportAt + 12, static_cast<std::uint32_t>(importOffset) ^ fields.mask);
    return out.bytes;
}

}

TEST(legacySummaryStandard) {
    auto header = parseLegacyPackageHeader(writePackage({}), "/Game/Test", EGame::GAME_UE4_26);
    REQUIRE(header.has_value());
    REQUIRE(header->common.names.size() == 1);
    CHECK(header->common.names[0].getText() == "None");
    CHECK(header->imports.size() == 1);
}

TEST(towerOfFantasySummaryMasksOffsets) {
    SummaryFields fields;
    fields.mask = 0xEEB2CEC7;
    auto header = parseLegacyPackageHeader(writePackage(fields), "/Game/Test", EGame::GAME_TowerOfFantasy);
    REQUIRE(header.has_value());
    CHECK(header->common.names.size() == 1);
    CHECK(header->imports.size() == 1);
    CHECK(!parseLegacyPackageHeader(writePackage(fields), "/Game/Test", EGame::GAME_UE4_26).has_value());
}

TEST(ashEchoesSummaryTag) {
    SummaryFields fields;
    fields.tag = 0x56DE5ECA;
    CHECK(parseLegacyPackageHeader(writePackage(fields), "/Game/Test", EGame::GAME_AshEchoes).has_value());
    CHECK(!parseLegacyPackageHeader(writePackage({.tag = 0x12345678}), "/Game/Test", EGame::GAME_AshEchoes).has_value());
}

TEST(valorantSummarySkipsGuidPadding) {
    SummaryFields fields;
    fields.extraAfterGuid = 8;
    auto header = parseLegacyPackageHeader(writePackage(fields), "/Game/Test", EGame::GAME_Valorant_PRE_11_2);
    REQUIRE(header.has_value());
    CHECK(header->common.names.size() == 1);
    CHECK(!parseLegacyPackageHeader(writePackage(fields), "/Game/Test", EGame::GAME_UE4_26).has_value());
}

TEST(seaOfThievesSummarySkipsBulkDataPadding) {
    SummaryFields fields;
    fields.extraBeforeBulkData = 6;
    auto header = parseLegacyPackageHeader(writePackage(fields), "/Game/Test", EGame::GAME_SeaOfThieves);
    REQUIRE(header.has_value());
    CHECK(header->common.bulkDataStartOffset == 0);
    auto standard = parseLegacyPackageHeader(writePackage(fields), "/Game/Test", EGame::GAME_UE4_26);
    REQUIRE(standard.has_value());
    CHECK(standard->common.bulkDataStartOffset != 0);
}
