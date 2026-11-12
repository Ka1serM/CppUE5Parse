#include "Conversion/Materials/MaterialShaderCompiler.h"

#include <utility>

#include "Conversion/Shaders/MaterialLayout.h"
#include "Conversion/Shaders/ShaderCompiler.h"

namespace ue5parse::conversion {

ConversionResult<CompiledMaterialShader> compileMaterialShader(const MaterialData& data, const ShaderData& shader, SpirvCache& cache) {
    ConversionResult<CompiledShader> compiled = compileShader(shader.code, shader.hash, data.game, MaterialLayout(*data.uniformExpressions, data.game), cache);
    if (!compiled)
        return std::unexpected(std::move(compiled.error()));
    return CompiledMaterialShader{
        .type = std::string(shader.type), .stage = shader.stage, .spirv = std::move(compiled->spirv), .reflection = std::move(compiled->reflection)};
}

}
