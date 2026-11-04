#include "Engine/Source/Runtime/D3D12RHI/D3D12Shaders.h"

#include <cstring>

namespace ue5parse::runtime {

namespace {

constexpr std::size_t ContainerHeaderSize = 32;
constexpr std::size_t ContainerSizeOffset = 24;

}

Result<void> FD3D12ShaderData::Read(std::span<const uint8> InCode, EGame Game) {
    FMemoryReaderView Ar(std::as_bytes(InCode));
    Ar.SetGame(Game);
    Ar << ShaderResourceTable;
    if (Ar.IsError())
        return std::unexpected(Ar.GetError());

    FShaderCodeReader Reader(InCode);
    Result<uint32> ActualSize = Reader.GetActualShaderCodeSize();
    if (!ActualSize)
        return std::unexpected(ActualSize.error());
    std::size_t Offset = static_cast<std::size_t>(Ar.Tell());
    if (*ActualSize < Offset + ContainerHeaderSize || std::memcmp(InCode.data() + Offset, "DXBC", 4) != 0)
        return fail(ErrorCode::BadMagic, "FD3D12ShaderData.Container", Offset);
    uint32 ContainerSize = 0u;
    std::memcpy(&ContainerSize, InCode.data() + Offset + ContainerSizeOffset, sizeof(ContainerSize));
    if (ContainerSize == 0u || ContainerSize > *ActualSize - Offset)
        return fail(ErrorCode::ReadPastEnd, "FD3D12ShaderData.Container", Offset);
    Code = InCode.subspan(Offset, ContainerSize);

    Result<std::span<const uint8>> Names = Reader.FindOptionalData(FShaderCodeUniformBuffers::Key);
    if (!Names)
        return std::unexpected(Names.error());
    if (Names->empty())
        return {};
    FMemoryReaderView NamesAr(std::as_bytes(*Names));
    NamesAr << UniformBufferNames;
    if (NamesAr.IsError())
        return std::unexpected(NamesAr.GetError());
    return {};
}

}
