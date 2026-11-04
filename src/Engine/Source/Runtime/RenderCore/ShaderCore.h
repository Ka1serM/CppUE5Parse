#pragma once

#include <span>

#include "CppUE5Parse/Core/Result.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FShaderResourceTable {
    uint32 ResourceTableBits = 0u;
    TArray<uint32> ShaderResourceViewMap;
    TArray<uint32> SamplerMap;
    TArray<uint32> UnorderedAccessViewMap;
    TArray<uint32> ResourceTableLayoutHashes;
    TArray<uint32> TextureMap;
    TArray<uint32> ResourceCollectionMap;
};

FArchive& operator<<(FArchive& Ar, FShaderResourceTable& Table);

Result<std::span<const uint32>> GetResourceTableEntries(std::span<const uint32> ResourceMap);

struct FShaderCodeUniformBuffers {
    static constexpr uint8 Key = 'u';
};

class FShaderCodeReader {
public:
    explicit FShaderCodeReader(std::span<const uint8> InShaderCode) : ShaderCode(InShaderCode) {}

    Result<uint32> GetOptionalDataSize() const;
    Result<uint32> GetActualShaderCodeSize() const;
    Result<std::span<const uint8>> FindOptionalData(uint8 Key) const;

private:
    std::span<const uint8> ShaderCode;
};

}
