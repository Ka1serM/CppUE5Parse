#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Conversion/Materials/MaterialData.h"
#include "Conversion/Materials/MaterialEvaluator.h"
#include "Conversion/Materials/MaterialShaderCompiler.h"
#include "Conversion/Spirv/ExpressionGraph.h"

namespace ue5parse::conversion {

struct SurfaceAttributes {
    std::array<ExprId, 3> baseColor{};
    std::array<ExprId, 3> normal{};
    std::array<ExprId, 3> emission{};
    ExprId roughness = 0;
    ExprId metallic = 0;
    ExprId specular = 0;
    std::optional<ExprId> opacity;
    std::optional<std::array<ExprId, 3>> refraction;
    std::optional<std::array<ExprId, 3>> displacement;
};

using SurfaceOverride = std::function<std::optional<SurfaceAttributes>(MaterialEvaluator&, const std::vector<CompiledMaterialShader>&, BlendMode)>;

SurfaceAttributes readSurface(MaterialEvaluator& evaluator, const std::vector<CompiledMaterialShader>& shaders, BlendMode blendMode,
                              const SurfaceOverride& surfaceOverride = {});

std::vector<std::string> absentValuesInSurface(const MaterialData& data, const std::vector<CompiledMaterialShader>& shaders,
                                               const SurfaceOverride& surfaceOverride);

}
