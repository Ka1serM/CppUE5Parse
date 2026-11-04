#include "Engine/Source/Runtime/RenderCore/ShaderCore.h"

#include <algorithm>
#include <cstring>

#include "Engine/Source/Runtime/RHI/RHIDefinitions.h"

namespace ue5parse::runtime {

namespace {

constexpr std::size_t OptionalDataHeaderSize = sizeof(uint8) + sizeof(uint32);

uint32 ReadUint32(std::span<const uint8> Bytes, std::size_t Offset) {
    uint32 Value = 0u;
    std::memcpy(&Value, Bytes.data() + Offset, sizeof(Value));
    return Value;
}

}

FArchive& operator<<(FArchive& Ar, FShaderResourceTable& Table) {
    Ar << Table.ResourceTableBits;
    Ar << Table.ShaderResourceViewMap;
    Ar << Table.SamplerMap;
    Ar << Table.UnorderedAccessViewMap;
    Ar << Table.ResourceTableLayoutHashes;
    Ar << Table.TextureMap;
    if (Ar.GetGame() >= EGame::GAME_UE5_5)
        Ar << Table.ResourceCollectionMap;
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

Result<uint32> FShaderCodeReader::GetOptionalDataSize() const {
    if (ShaderCode.size() < sizeof(uint32))
        return fail(ErrorCode::ReadPastEnd, "FShaderCodeReader.GetOptionalDataSize");
    uint32 Size = ReadUint32(ShaderCode, ShaderCode.size() - sizeof(uint32));
    if (Size < sizeof(uint32) || Size > ShaderCode.size())
        return fail(ErrorCode::InvalidData, "FShaderCodeReader.GetOptionalDataSize", ShaderCode.size() - sizeof(uint32));
    return Size;
}

Result<uint32> FShaderCodeReader::GetActualShaderCodeSize() const {
    return GetOptionalDataSize().transform([&](uint32 Size) { return static_cast<uint32>(ShaderCode.size()) - Size; });
}

Result<std::span<const uint8>> FShaderCodeReader::FindOptionalData(uint8 Key) const {
    Result<uint32> OptionalDataSize = GetOptionalDataSize();
    if (!OptionalDataSize)
        return std::unexpected(OptionalDataSize.error());
    std::size_t End = ShaderCode.size() - sizeof(uint32);
    std::size_t Current = ShaderCode.size() - *OptionalDataSize;
    while (Current + OptionalDataHeaderSize <= End) {
        uint32 Size = ReadUint32(ShaderCode, Current + sizeof(uint8));
        std::size_t Data = Current + OptionalDataHeaderSize;
        if (Size > End - Data)
            return fail(ErrorCode::InvalidData, "FShaderCodeReader.FindOptionalData", Current);
        if (ShaderCode[Current] == Key)
            return ShaderCode.subspan(Data, Size);
        Current = Data + Size;
    }
    return std::span<const uint8>();
}

}
