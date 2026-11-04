#pragma once

#include <array>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Conversion/Spirv/ExpressionGraph.h"
#include "Conversion/Spirv/ExpressionProgram.h"

namespace ue5parse::conversion {

class ProgramWriter {
public:
    ProgramWriter(const ExpressionGraph& graph, std::span<const uint32_t> parameterWidths);

    uint32_t write(ExprId value);
    uint32_t write(std::span<const ExprId, 3> value);
    const std::optional<std::string>& getFailure() const { return failure; }
    ExpressionProgram finish();

private:
    using Lanes = std::vector<ExprId>;
    enum class Shape { None, Uniform, Constant, Leaf, Operation };
    struct Clamp {
        ExprId value, low, high;
    };
    struct DotProduct {
        Lanes a, b, scale;
    };
    using Key = std::tuple<ProgramOp, uint8_t, uint16_t, uint32_t, uint32_t, uint32_t>;

    std::vector<uint32_t> uses() const;
    uint32_t emit(ExprId id);
    uint32_t emitNode(ExprId id);
    std::optional<uint32_t> emitSemantic(uint32_t index);
    std::optional<uint32_t> semanticLanes(const Lanes& lanes) const;
    uint32_t emit(const Lanes& lanes);
    uint32_t emitLanes(const Lanes& lanes);
    uint32_t rememberVectorLanes(const Lanes& lanes, uint32_t vector);
    uint32_t operand(const Lanes& lanes);
    Shape shape(const Lanes& lanes);
    Shape computeShape(const Lanes& lanes);
    std::array<Lanes, 3> operands(const Lanes& lanes) const;

    std::optional<Clamp> clamp(ExprId id) const;
    std::optional<uint32_t> smoothstep(ExprId id);
    std::optional<Lanes> sumTerms(ExprId id) const;
    std::optional<Lanes> squares(ExprId id) const;
    Lanes factors(ExprId id) const;
    std::optional<DotProduct> dotProduct(ExprId id);
    std::optional<std::pair<Lanes, Lanes>> pairLanes(const std::vector<std::pair<ExprId, ExprId>>& pairs);
    std::optional<std::pair<Lanes, Lanes>> crossProduct(const Lanes& lanes);
    std::optional<Lanes> normalized(const Lanes& lanes) const;

    uint32_t mask(ExprId condition);
    uint32_t leaf(const ExprNode& node);
    uint32_t constant(std::span<const float> values);
    uint32_t constant(float value) { return constant(std::span(&value, 1)); }
    uint32_t extract(uint32_t vector, uint32_t component);
    uint32_t instruction(ProgramOp op, uint32_t width, uint32_t index = 0, uint32_t a = 0, uint32_t b = 0, uint32_t c = 0);
    std::optional<uint32_t> simplify(ProgramOp op, uint32_t width, uint32_t a, uint32_t b, uint32_t c);
    std::optional<uint32_t> tangentToWorld(uint32_t sum);
    std::array<uint32_t, 3> withoutScale(std::array<uint32_t, 3> factors) const;
    bool isGeometry(uint32_t id, GeometryAttribute attribute) const;
    bool isConstant(uint32_t id, float value) const;
    uint32_t width(const uint32_t id) const { return program.instructions[id].width; }
    uint32_t fail(std::string message);

    const ExpressionGraph& graph;
    std::vector<uint32_t> parameterWidths;
    ExpressionProgram program;
    std::map<Key, uint32_t> instructions;
    std::map<std::vector<float>, uint32_t> constants;
    std::unordered_map<ExprId, uint32_t> emitted;
    std::unordered_map<ExprId, std::pair<uint32_t, uint32_t>> vectorComponents;
    std::unordered_map<uint32_t, std::optional<uint32_t>> semanticResults;
    std::unordered_set<uint32_t> semanticActive;
    std::map<Lanes, uint32_t> vectors;
    std::map<Lanes, Shape> shapes;
    std::optional<std::string> failure;
};

}
