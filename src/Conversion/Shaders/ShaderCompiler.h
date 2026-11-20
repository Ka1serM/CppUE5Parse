#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>

#include "Conversion/Error.h"
#include "Conversion/Shaders/MaterialLayout.h"
#include "Conversion/Shaders/ShaderReflection.h"
#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/Core/Hash/ShaderHash.h"

namespace ue5parse::conversion {

using Spirv = std::shared_ptr<const std::vector<std::uint32_t>>;

struct CompiledShader {
    Spirv spirv;
    ShaderReflection reflection;
};

class SpirvCache {
public:
    ConversionResult<Spirv> convert(const runtime::FShaderCodeHash& hash, std::span<const std::uint8_t> container);

private:
    struct Entry {
        std::once_flag converted;
        ConversionResult<Spirv> spirv;
    };

    std::mutex mutex;
    std::unordered_map<runtime::FShaderCodeHash, Entry, runtime::FShaderCodeHashHasher> entries;
};

ConversionResult<CompiledShader> compileShader(std::span<const std::uint8_t> code, const runtime::FShaderCodeHash& hash, EGame game, const MaterialLayout& material,
                                               SpirvCache& cache);

}
