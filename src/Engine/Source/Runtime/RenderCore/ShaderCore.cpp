#include "Engine/Source/Runtime/RenderCore/ShaderCore.h"

#include <algorithm>
#include <cstring>

#include "Engine/Source/Runtime/RHI/RHIDefinitions.h"

namespace ue5parse::runtime {

namespace {

uint32 ReadUint32(std::span<const uint8> Bytes, std::size_t Offset) {
    uint32 Value = 0u;
    std::memcpy(&Value, Bytes.data() + Offset, sizeof(Value));
    return Value;
}

//! UE 4.11 to 4.14 end the optional data with its big-endian uint16 size and give each entry a uint8 size.
uint32 ReadSize(std::span<const uint8> Bytes, std::size_t Offset, std::size_t Size) {
    if (Size == sizeof(uint32))
        return ReadUint32(Bytes, Offset);
    if (Size == sizeof(uint16))
        return (static_cast<uint32>(Bytes[Offset]) << 8) | Bytes[Offset + 1];
    return Bytes[Offset];
}

}

FArchive& operator<<(FArchive& Ar, FShaderResourceTable& Table) {
    Ar << Table.ResourceTableBits;
    Ar << Table.ShaderResourceViewMap;
    Ar << Table.SamplerMap;
    Ar << Table.UnorderedAccessViewMap;
    Ar << Table.ResourceTableLayoutHashes;
    //! Before UE 4.12 the D3D11 shader compiler merges the texture map into the SRV map.
    if (Ar.GetGame() >= EGame::GAME_UE4_12)
        Ar << Table.TextureMap;
    if (Ar.GetGame() >= EGame::GAME_UE5_5)
        Ar << Table.ResourceCollectionMap;
    if (Ar.GetGame() >= EGame::GAME_UE5_9)
        Ar << Table.UniformBlockMap;
    return Ar;
}

Result<std::span<const uint32>> GetResourceTableEntries(std::span<const uint32> ResourceMap) {
    std::size_t First = ResourceMap.size();
    for (uint32 Offset : ResourceMap)
        if (Offset != 0u)
            First = std::min<std::size_t>(First, Offset);
    if (First > ResourceMap.size())
        return fail(ErrorCode::InvalidData, "GetResourceTableEntries.Offset");
    std::span<const uint32> Entries = ResourceMap.subspan(First);
    return Entries.first(static_cast<std::size_t>(std::ranges::find(Entries, FRHIResourceTableEntry::GetEndOfStreamToken()) - Entries.begin()));
}

std::size_t FShaderCodeReader::GetSizeFieldSize() const {
    return Game >= EGame::GAME_UE4_15 ? sizeof(uint32) : sizeof(uint16);
}

std::size_t FShaderCodeReader::GetEntrySizeFieldSize() const {
    return Game >= EGame::GAME_UE4_15 ? sizeof(uint32) : sizeof(uint8);
}

//! Before UE 4.11 the shader code has no optional data; the D3D11 shader compiler writes bGlobalUniformBufferUsed in its last byte.
Result<uint32> FShaderCodeReader::GetOptionalDataSize() const {
    const std::size_t SizeFieldSize = Game >= EGame::GAME_UE4_11 ? GetSizeFieldSize() : sizeof(uint8);
    if (ShaderCode.size() < SizeFieldSize)
        return fail(ErrorCode::ReadPastEnd, "FShaderCodeReader.GetOptionalDataSize");
    if (Game < EGame::GAME_UE4_11)
        return static_cast<uint32>(SizeFieldSize);
    uint32 Size = ReadSize(ShaderCode, ShaderCode.size() - SizeFieldSize, SizeFieldSize);
    if (Size < SizeFieldSize || Size > ShaderCode.size())
        return fail(ErrorCode::InvalidData, "FShaderCodeReader.GetOptionalDataSize", ShaderCode.size() - SizeFieldSize);
    return Size;
}

Result<uint32> FShaderCodeReader::GetActualShaderCodeSize() const {
    return GetOptionalDataSize().transform([&](uint32 Size) { return static_cast<uint32>(ShaderCode.size()) - Size; });
}

Result<std::span<const uint8>> FShaderCodeReader::FindOptionalData(uint8 Key) const {
    if (Game < EGame::GAME_UE4_11)
        return std::span<const uint8>();
    Result<uint32> OptionalDataSize = GetOptionalDataSize();
    if (!OptionalDataSize)
        return std::unexpected(OptionalDataSize.error());
    const std::size_t EntrySizeFieldSize = GetEntrySizeFieldSize();
    const std::size_t HeaderSize = sizeof(uint8) + EntrySizeFieldSize;
    std::size_t End = ShaderCode.size() - GetSizeFieldSize();
    std::size_t Current = ShaderCode.size() - *OptionalDataSize;
    while (Current + HeaderSize <= End) {
        uint32 Size = ReadSize(ShaderCode, Current + sizeof(uint8), EntrySizeFieldSize);
        std::size_t Data = Current + HeaderSize;
        if (Size > End - Data)
            return fail(ErrorCode::InvalidData, "FShaderCodeReader.FindOptionalData", Current);
        if (ShaderCode[Current] == Key)
            return ShaderCode.subspan(Data, Size);
        Current = Data + Size;
    }
    return std::span<const uint8>();
}

}
