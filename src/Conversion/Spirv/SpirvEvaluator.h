#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "Conversion/Spirv/ExpressionGraph.h"

namespace ue5parse::conversion {

class ShaderInputs {
public:
    virtual ~ShaderInputs() = default;
    virtual ExprId constantBuffer(uint32_t reg, uint32_t offset) = 0;
    virtual ExprId input(uint32_t location, uint32_t component) = 0;
    virtual ExprId frontFacing() = 0;
    virtual ExprId fragCoord(uint32_t component) = 0;
    virtual std::array<ExprId, 4> sample(uint32_t reg, std::span<const ExprId> coordinate) = 0;
    virtual std::array<ExprId, 4> fetch(uint32_t reg, ExprId index) = 0;
    virtual std::array<ExprId, 3> textureSize(uint32_t reg, ExprId lod) = 0;
};

struct ShaderOutputs {
    std::vector<std::array<ExprId, 4>> targets;
    std::array<ExprId, 4> position{};
    ExprId discard = 0;
    std::vector<std::string> notes;
};

inline constexpr uint32_t maxUnknownLoopIterations = 1;
static_assert(maxUnknownLoopIterations >= 1);

ShaderOutputs evaluateSpirv(std::span<const uint32_t> words, ExpressionGraph& graph, ShaderInputs& inputs);

}
