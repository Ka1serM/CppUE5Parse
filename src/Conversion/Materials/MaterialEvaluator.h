#pragma once

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "Conversion/Materials/MaterialData.h"
#include "Conversion/Materials/MaterialShaderCompiler.h"
#include "Conversion/Materials/Warnings.h"
#include "Conversion/Spirv/ExpressionGraph.h"
#include "Conversion/Spirv/SpirvEvaluator.h"

namespace ue5parse::conversion {

using EngineValues = std::map<std::string, std::vector<float>>;
using EngineSymbols = std::set<std::string>;
using Interpolants = std::map<std::pair<std::string, uint32_t>, std::vector<ExprId>>;

struct BoundTexture {
    uint32_t program = 0;
    std::array<int, 3> size{};
};
using TextureSlots = std::map<std::pair<runtime::EMaterialTextureParameterType, uint32_t>, BoundTexture>;

enum class VertexProjection { Identity, Camera };

class MaterialEvaluator {
public:
    explicit MaterialEvaluator(const MaterialData& data);

    ExpressionGraph& getGraph() { return graph; }
    const MaterialData& getMaterial() const { return data; }
    const Warnings& getWarnings() const { return warnings; }
    const std::vector<NumericParameter>& getCollectionParameters() const { return collectionParameters; }
    ShaderOutputs evaluate(const CompiledMaterialShader& shader, const EngineValues& engine, const EngineSymbols& symbols = {},
                           VertexProjection projection = VertexProjection::Identity);

private:
    Interpolants interpolants;
    const MaterialData& data;
    ExpressionGraph graph;
    Warnings warnings;
    std::vector<ExprId> material;
    TextureSlots textureSlots;
    std::vector<NumericParameter> collectionParameters;
};

EngineValues viewDefaults();

}
