#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Conversion/Error.h"
#include "Conversion/Materials/MaterialData.h"
#include "Conversion/Shaders/ShaderCompiler.h"
#include "Conversion/Shaders/ShaderReflection.h"

namespace ue5parse::conversion {

struct CompiledMaterialShader {
    std::string type;
    ShaderStage stage = ShaderStage::Pixel;
    Spirv spirv;
    ShaderReflection reflection;
};

ConversionResult<CompiledMaterialShader> compileMaterialShader(const MaterialData& data, const ShaderData& shader, SpirvCache& cache);

}
