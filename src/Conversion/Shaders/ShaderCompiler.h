#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "Conversion/Error.h"
#include "Conversion/Shaders/MaterialLayout.h"
#include "Conversion/Shaders/ShaderReflection.h"
#include "CppUE5Parse/Core/Versions.h"

namespace ue5parse::conversion {

struct CompiledShader {
    std::vector<std::uint32_t> spirv;
    ShaderReflection reflection;
};

ConversionResult<CompiledShader> compileShader(std::span<const std::uint8_t> code, EGame game, const MaterialLayout& material);

}
