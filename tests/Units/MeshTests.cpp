#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Engine/RawIndexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/PositionVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/TextureResource.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::runtime::FMemoryReaderView;
using ue5parse::runtime::FPositionVertexBuffer;
using ue5parse::runtime::FRawStaticIndexBuffer;
using ue5parse::runtime::FTexture2DMipMap;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    std::vector<std::byte> bytes;
};

FPositionVertexBuffer readPositions(const Writer& data, EGame game) {
    FMemoryReaderView reader(data.bytes);
    reader.SetGame(game);
    FPositionVertexBuffer buffer;
    buffer.Serialize(reader);
    return buffer;
}

}

TEST(daysGonePackedPositions) {
    Writer data;
    data.put<std::uint32_t>(4);
    data.put<std::uint32_t>(1);
    data.put<std::int32_t>(4);
    data.put<std::int32_t>(1);
    data.put<std::uint32_t>(1 | (2 << 10) | (3 << 20));
    FPositionVertexBuffer buffer = readPositions(data, EGame::GAME_DaysGone);
    REQUIRE(buffer.GetVertexData().Num() == 1);
    CHECK(buffer.GetVertexData()[0].X == 1.0f);
    CHECK(buffer.GetVertexData()[0].Y == 2.0f);
    CHECK(buffer.GetVertexData()[0].Z == 3.0f);
}

TEST(rocoKingdomWorldSignedShortPositions) {
    Writer data;
    data.put<std::uint32_t>(8);
    data.put<std::uint32_t>(1);
    data.put<std::int32_t>(8);
    data.put<std::int32_t>(1);
    data.put<std::int16_t>(100);
    data.put<std::int16_t>(-50);
    data.put<std::int16_t>(25);
    data.put<std::int16_t>(50);
    FPositionVertexBuffer buffer = readPositions(data, EGame::GAME_RocoKingdomWorld);
    REQUIRE(buffer.GetVertexData().Num() == 1);
    CHECK(buffer.GetVertexData()[0].X == 2.0f);
    CHECK(buffer.GetVertexData()[0].Y == -1.0f);
    CHECK(buffer.GetVertexData()[0].Z == 0.5f);
}

TEST(rocoKingdomWorldRejectsUnknownStride) {
    Writer data;
    data.put<std::uint32_t>(6);
    data.put<std::uint32_t>(0);
    FMemoryReaderView reader(data.bytes);
    reader.SetGame(EGame::GAME_RocoKingdomWorld);
    FPositionVertexBuffer buffer;
    buffer.Serialize(reader);
    CHECK(reader.IsError());
}

TEST(worldofJadeDynastyMaskedPositionCount) {
    Writer data;
    data.put<std::uint32_t>(12 ^ 0xDBB1054F);
    data.put<std::uint32_t>(1 << 9);
    data.put<std::int32_t>(12);
    data.put<std::int32_t>(1);
    data.put(1.0f);
    data.put(2.0f);
    data.put(3.0f);
    FPositionVertexBuffer buffer = readPositions(data, EGame::GAME_WorldofJadeDynasty);
    CHECK(buffer.GetNumVertices() == 1);
    REQUIRE(buffer.GetVertexData().Num() == 1);
    CHECK(buffer.GetVertexData()[0].Z == 3.0f);
}

TEST(pubgIndexDeltas) {
    Writer data;
    data.put<std::int32_t>(0);
    data.put<std::int32_t>(1);
    data.put<std::int32_t>(6);
    data.put<std::int16_t>(5);
    data.put<std::int16_t>(3);
    data.put<std::int16_t>(-2);
    FMemoryReaderView reader(data.bytes);
    reader.SetGame(EGame::GAME_PlayerUnknownsBattlegrounds);
    FRawStaticIndexBuffer buffer;
    buffer.Serialize(reader);
    REQUIRE(!reader.IsError());
    REQUIRE(buffer.GetNumIndices() == 3);
    CHECK(buffer.GetIndex(0) == 5);
    CHECK(buffer.GetIndex(1) == 8);
    CHECK(buffer.GetIndex(2) == 6);
}

TEST(worldofJadeDynastyMaskedMipSize) {
    Writer data;
    if (EGame::GAME_WorldofJadeDynasty < EGame::GAME_UE5_0)
        data.put<std::int32_t>(1);
    data.put<std::uint32_t>(64 ^ 0xA537EA93);
    data.put<std::int32_t>(32);
    data.put<std::uint32_t>(std::byteswap(1u));
    FMemoryReaderView reader(data.bytes);
    reader.SetGame(EGame::GAME_WorldofJadeDynasty);
    FTexture2DMipMap mip;
    mip.Serialize(reader, false);
    REQUIRE(!reader.IsError());
    CHECK(mip.SizeX == 64);
    CHECK(mip.SizeY == 32);
    CHECK(mip.SizeZ == 1);
}

TEST(borderlands3ShortMipSize) {
    Writer data;
    data.put<std::int32_t>(1);
    data.put<std::uint16_t>(128);
    data.put<std::uint16_t>(64);
    data.put<std::uint16_t>(1);
    FMemoryReaderView reader(data.bytes);
    reader.SetGame(EGame::GAME_Borderlands3);
    FTexture2DMipMap mip;
    mip.Serialize(reader, false);
    REQUIRE(!reader.IsError());
    CHECK(mip.SizeX == 128);
    CHECK(mip.SizeY == 64);
    CHECK(reader.Tell() == static_cast<std::int64_t>(data.bytes.size()));
}
