#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Engine/Components/InstancedStaticMeshComponent.h"
#include "Engine/Source/Runtime/Engine/RawIndexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/PositionVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/TextureResource.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::runtime::FInstancedStaticMeshInstanceData;
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

template <typename T>
void putMatrix(Writer& data, T translationZ) {
    for (int i = 0; i < 16; i++)
        data.put<T>(i == 14 ? translationZ : static_cast<T>(i % 5 == 0 ? 1 : 0));
}

void putPadding(Writer& data, int size) {
    for (int i = 0; i < size; i++)
        data.put<std::uint8_t>(0xCD);
}

struct InstanceRead {
    ue5parse::runtime::TArray<FInstancedStaticMeshInstanceData> instances;
    bool error = false;
    bool atEnd = false;
};

InstanceRead readInstances(const Writer& data, EGame game) {
    FMemoryReaderView reader(data.bytes);
    reader.SetGame(game);
    InstanceRead result;
    ue5parse::runtime::SerializePerInstanceData(reader, result.instances);
    result.error = reader.IsError();
    result.atEnd = reader.Tell() == static_cast<std::int64_t>(data.bytes.size());
    return result;
}

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

TEST(codeVeinInstanceSkipsUVBias) {
    Writer data;
    data.put<std::int32_t>(16 * 4 + 16);
    data.put<std::int32_t>(1);
    putMatrix<float>(data, 7.0f);
    putPadding(data, 16);
    InstanceRead read = readInstances(data, EGame::GAME_CodeVein);
    REQUIRE(!read.error);
    REQUIRE(read.instances.Num() == 1);
    CHECK(read.instances[0].Transform.M[3][2] == 7.0);
    CHECK(read.atEnd);
}

TEST(silentHill2RemakeInstanceSkipsTwoUVBiases) {
    Writer data;
    data.put<std::int32_t>(16 * 8 + 32);
    data.put<std::int32_t>(1);
    putMatrix<double>(data, 5.0);
    putPadding(data, 32);
    InstanceRead read = readInstances(data, EGame::GAME_SilentHill2Remake);
    REQUIRE(!read.error);
    REQUIRE(read.instances.Num() == 1);
    CHECK(read.instances[0].Transform.M[3][2] == 5.0);
    CHECK(read.atEnd);
}

TEST(hogwartsLegacyInstanceHasVariableSize) {
    Writer data;
    data.put<std::int32_t>(123);
    data.put<std::int32_t>(2);
    putMatrix<float>(data, 1.0f);
    data.put<std::int32_t>(2);
    putPadding(data, 2 * 4 + 4);
    putMatrix<float>(data, 2.0f);
    data.put<std::int32_t>(0);
    putPadding(data, 4);
    InstanceRead read = readInstances(data, EGame::GAME_HogwartsLegacy);
    REQUIRE(!read.error);
    REQUIRE(read.instances.Num() == 2);
    CHECK(read.instances[1].Transform.M[3][2] == 2.0);
    CHECK(read.atEnd);
}

TEST(pubgInstanceOfHundredBytes) {
    Writer data;
    data.put<std::int32_t>(100);
    data.put<std::int32_t>(1);
    putMatrix<float>(data, 3.0f);
    putPadding(data, 16 + 20);
    InstanceRead read = readInstances(data, EGame::GAME_PlayerUnknownsBattlegrounds);
    REQUIRE(!read.error);
    REQUIRE(read.instances.Num() == 1);
    CHECK(read.instances[0].Transform.M[3][2] == 3.0);
    CHECK(read.atEnd);
}

TEST(stalker2InstanceIgnoresElementSize) {
    Writer data;
    data.put<std::int32_t>(0);
    data.put<std::int32_t>(1);
    putMatrix<double>(data, 4.0);
    InstanceRead read = readInstances(data, EGame::GAME_Stalker2);
    REQUIRE(!read.error);
    REQUIRE(read.instances.Num() == 1);
    CHECK(read.instances[0].Transform.M[3][2] == 4.0);
    CHECK(read.atEnd);
}

TEST(throneAndLibertySkipsHalfInstances) {
    Writer data;
    data.put<std::int32_t>(16);
    data.put<std::int32_t>(2);
    putPadding(data, 2 * 16 + 24);
    InstanceRead read = readInstances(data, EGame::GAME_ThroneAndLiberty);
    REQUIRE(!read.error);
    CHECK(read.instances.Num() == 0);
    CHECK(read.atEnd);
}

TEST(throneAndLibertyRejectsUnknownElementSize) {
    Writer data;
    data.put<std::int32_t>(24);
    data.put<std::int32_t>(0);
    InstanceRead read = readInstances(data, EGame::GAME_ThroneAndLiberty);
    CHECK(read.error);
}
