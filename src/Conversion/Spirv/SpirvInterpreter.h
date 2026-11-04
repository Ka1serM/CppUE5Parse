#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>

#include <spirv/unified1/GLSL.std.450.h>
#include <spirv/unified1/spirv.hpp>

#include "Conversion/Spirv/SpirvEvaluator.h"

namespace ue5parse::conversion {

struct Scalar {
    ExprId expr = 0;
    std::optional<uint32_t> bits;
};

struct Pointer {
    uint32_t variable = 0;
    uint32_t type = 0;
    uint32_t component = 0;
    uint32_t offset = 0;
};

struct Handle {
    uint32_t binding = 0;
};

using Value = std::variant<std::vector<Scalar>, Pointer, Handle>;
using Memory = std::map<uint32_t, std::vector<Scalar>>;

struct Type {
    spv::Op op = spv::OpNop;
    uint32_t element = 0;
    uint32_t count = 0;
    std::vector<uint32_t> members;
    bool isSigned = false;
    spv::StorageClass storage = spv::StorageClassMax;
};

struct Variable {
    uint32_t type = 0;
    spv::StorageClass storage = spv::StorageClassMax;
    std::optional<uint32_t> initializer;
};

struct Instruction {
    spv::Op op;
    std::span<const uint32_t> operands;
};

struct Incoming {
    uint32_t predecessor = 0;
    ExprId condition = 0;
    uint32_t truePredecessor = 0;
    uint32_t falsePredecessor = 0;
    bool fork = false;
    bool phisResolved = false;
};

struct LoopExit {
    ExprId condition;
    Memory memory;
    std::unordered_map<uint32_t, Value> values;
    uint32_t predecessor;
    uint32_t iteration;
};

struct Loop {
    uint32_t header = 0, merge = 0, continueTarget = 0;
    int forkDepth = 0;
    uint32_t iterations = 0;
    uint32_t unknownIterations = 0;
    ExprId entryPath = 0;
    std::vector<LoopExit> exits;
};

enum class Bitwise { And, Or, Xor };
class SpirvInterpreter {
public:
    SpirvInterpreter(const std::span<const uint32_t> words, ExpressionGraph& graph, ShaderInputs& inputs) : graph(graph), inputs(inputs) { parse(words); }

    ShaderOutputs run();

private:
    void parse(const std::span<const uint32_t> words);

    void decorate(const uint32_t id, const spv::Decoration decoration, const std::span<const uint32_t> values);

    static std::string literalString(const std::span<const uint32_t> words);

    ExprId zero() { return graph.constant(0.0f); }

    Scalar constantScalar(const uint32_t type, const uint32_t bits);

    Scalar integer(const int64_t value) { return {graph.constant(static_cast<float>(value)), static_cast<uint32_t>(value)}; }
    Scalar floating(const ExprId expr) const;

    uint32_t componentCount(const uint32_t typeId) const;

    const std::vector<Scalar>& scalars(const uint32_t id);

    Scalar scalar(const uint32_t id);

    template <typename T>
    std::optional<T> get(const uint32_t id) {
        const auto found = values.find(id);
        if (found == values.end()) {
            graph.fail("The SPIR-V value %" + std::to_string(id) + " is not available.");
            return std::nullopt;
        }
        if (const auto* result = std::get_if<T>(&found->second))
            return *result;
        graph.fail("The SPIR-V value %" + std::to_string(id) + " has an unexpected kind.");
        return std::nullopt;
    }

    static void append(std::vector<Scalar>& target, const std::vector<Scalar>& source) { target.insert(target.end(), source.begin(), source.end()); }

    std::vector<Scalar> initialValue(const Variable& variable);

    std::optional<Incoming> runRegion(uint32_t block, const uint32_t stop, Incoming incoming, ExprId path);

    std::optional<Incoming> failRegion(std::string message);

    std::optional<Incoming> fork(const uint32_t header, const uint32_t trueBlock, const uint32_t falseBlock, const uint32_t merge, const ExprId condition,
                                 const ExprId path);

    bool leavesLoop(const uint32_t target, const Loop& loop) const;

    void recordExit(Loop& loop, const uint32_t block, const uint32_t target, const ExprId condition, const ExprId path);

    ExprId joinExits(Loop& loop, const std::optional<uint32_t> predecessor);

    void resolvePhis(const uint32_t block, const uint32_t predecessor, std::unordered_map<uint32_t, Value>& state);

    Value phi(const std::span<const uint32_t> operands, const Incoming& incoming);

    std::vector<Scalar> selectValues(const ExprId condition, const std::vector<Scalar>& a, const std::vector<Scalar>& b);

    Memory mergeMemory(const ExprId condition, const Memory& a, const Memory& b);

    auto floatOp(const ExprOp op) {
        return [this, op](const Scalar& a, const Scalar& b) { return graph.binary(op, a.expr, b.expr); };
    }

    auto truncatedQuotient() {
        return [this](const Scalar& a, const Scalar& b) { return graph.unary(ExprOp::Trunc, graph.div(a.expr, b.expr)); };
    }

    static constexpr int maxBitwiseBits = 16;

    auto bitwise(const Bitwise kind) {
        return [this, kind](const Scalar& a, const Scalar& b) {
            if (a.bits || b.bits) {
                const ExprId x = a.bits ? b.expr : a.expr;
                const uint32_t mask = a.bits ? *a.bits : *b.bits;
                const ExprId both = maskedBits(x, mask);
                if (kind == Bitwise::And)
                    return both;
                const ExprId sum = graph.add(x, graph.constant(static_cast<float>(mask)));
                return graph.sub(sum, kind == Bitwise::Or ? both : graph.mul(graph.constant(2.0f), both));
            }
            const std::optional<double> boundA = nonNegativeBound(a.expr), boundB = nonNegativeBound(b.expr);
            if (!boundA || !boundB || std::max(*boundA, *boundB) >= std::exp2(maxBitwiseBits))
                return graph.unresolved("bits of " + unresolvedNames(graph.add(a.expr, b.expr)));
            const int width = std::bit_width(static_cast<uint32_t>(std::max(*boundA, *boundB)));
            ExprId result = zero();
            for (int k = 0; k < width; ++k) {
                const ExprId x = bit(a.expr, k, *boundA), y = bit(b.expr, k, *boundB);
                const ExprId combined = kind == Bitwise::And  ? graph.min(x, y)
                                        : kind == Bitwise::Or ? graph.max(x, y)
                                                              : graph.unary(ExprOp::Abs, graph.sub(x, y));
                result = graph.add(result, graph.mul(combined, graph.constant(std::exp2(static_cast<float>(k)))));
            }
            return result;
        };
    }

    ExprId maskedBits(const ExprId x, const uint32_t mask);

    ExprId bit(const ExprId x, const int k, const double bound);

    std::optional<double> nonNegativeBound(const ExprId id);

    void execute(const Instruction& instruction, const ExprId path);

    void gather(const std::span<const uint32_t> o);

    ExprId truncatedRemainder(const ExprId a, const ExprId b) { return graph.sub(a, graph.mul(b, graph.unary(ExprOp::Trunc, graph.div(a, b)))); }

    Scalar boolean(const ExprId expr) const;

    Scalar bitcast(const uint32_t typeId, const Scalar& x);

    template <typename F>
    std::vector<Scalar> map1(const uint32_t a, F function) {
        std::vector<Scalar> result;
        for (const Scalar& x : scalars(a))
            result.push_back(function(x));
        return result;
    }

    template <typename F>
    std::vector<Scalar> map2(const uint32_t a, const uint32_t b, F function) {
        const std::vector<Scalar>& left = scalars(a);
        const std::vector<Scalar>& right = scalars(b);
        if (right.size() != 1 && right.size() != left.size()) {
            graph.fail("A binary operation has operands of different sizes.");
            return {};
        }
        std::vector<Scalar> result;
        result.reserve(left.size());
        for (size_t i = 0; i < left.size(); ++i)
            result.push_back(function(left[i], right[right.size() == 1 ? 0 : i]));
        return result;
    }

    void floatBinary(const std::span<const uint32_t> o, const ExprOp op);

    void compare(const std::span<const uint32_t> o, const ExprOp op, const bool swapped);

    static constexpr float hashModulus = 4096.0f;

    ExprId lowBits(const ExprId value);

    template <typename F>
    void shift(const std::span<const uint32_t> o, F function, const bool left) {
        values[o[1]] = map2(o[2], o[3], [&](const Scalar& a, const Scalar& b) {
            if (a.bits && b.bits) {
                const auto result = static_cast<uint32_t>(function(static_cast<int64_t>(*a.bits), static_cast<int64_t>(*b.bits)));
                return Scalar{graph.constant(static_cast<float>(result)), result};
            }
            if (!b.bits)
                return Scalar{graph.unresolved("bits of " + unresolvedNames(graph.add(a.expr, b.expr))), std::nullopt};
            const uint32_t amount = *b.bits & 31u;
            if (left)
                return Scalar{graph.mul(a.expr, graph.constant(std::exp2(static_cast<float>(amount)))), std::nullopt};
            const float scale = std::exp2(static_cast<float>(32u - amount)) / hashModulus;
            return Scalar{graph.unary(ExprOp::Floor, graph.mul(lowBits(a.expr), graph.constant(scale))), std::nullopt};
        });
    }

    template <typename F, typename G>
    void integerBinary(const std::span<const uint32_t> o, F function, G inexact) {
        const bool isSigned = types.at(elementType(o[0])).isSigned;
        values[o[1]] = map2(o[2], o[3], [&](const Scalar& a, const Scalar& b) {
            if (a.bits && b.bits) {
                const int64_t left = isSigned ? static_cast<int32_t>(*a.bits) : static_cast<int64_t>(*a.bits);
                const int64_t right = isSigned ? static_cast<int32_t>(*b.bits) : static_cast<int64_t>(*b.bits);
                const auto result = static_cast<uint32_t>(function(left, right));
                return Scalar{graph.constant(isSigned ? static_cast<float>(static_cast<int32_t>(result)) : static_cast<float>(result)), result};
            }
            return Scalar{inexact(a, b), std::nullopt};
        });
    }

    uint32_t elementType(const uint32_t typeId) const;

    std::vector<Scalar> glsl(const uint32_t type, const GLSLstd450 function, const std::span<const uint32_t> args);

    std::pair<uint32_t, uint32_t> compositeRange(const uint32_t composite, const std::span<const uint32_t> indices) const;

    Pointer accessChain(const uint32_t base, const std::span<const uint32_t> indices);

    void step(Pointer& pointer, const uint32_t index);

    void uniformOffsets(const uint32_t typeId, const uint32_t offset, std::vector<std::pair<uint32_t, uint32_t>>& result) const;

    Value load(const uint32_t pointerId);

    void store(const uint32_t pointerId, const std::vector<Scalar>& value);

    ExpressionGraph& graph;
    ShaderInputs& inputs;
    uint32_t entryBlock = 0;
    uint32_t glslSet = 0;
    uint32_t currentBlock = 0;
    std::unordered_map<uint32_t, Type> types;
    std::unordered_map<uint32_t, Value> values;
    std::unordered_map<uint32_t, uint32_t> valueTypes;
    std::map<uint32_t, Variable> variables;
    std::unordered_map<uint32_t, std::vector<Instruction>> blocks;
    std::unordered_map<uint32_t, uint32_t> locations, bindings, arrayStrides;
    std::unordered_map<uint32_t, spv::BuiltIn> builtins;
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> memberOffsets;
    std::unordered_map<ExprId, ExprId> lowBitsCache;
    std::unordered_map<ExprId, std::optional<double>> boundCache;
    Memory memory;

    static constexpr uint32_t maxLoopIterations = 1024;
    std::vector<Loop> loops;
    int forkDepth = 0;
    std::vector<ExprId> forkConditions;

    std::string unresolvedNames(const ExprId value) const;
    std::set<std::string> notes;
    ExprId discard = 0;
};

}
