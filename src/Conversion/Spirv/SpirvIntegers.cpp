#include "Conversion/Spirv/SpirvInterpreter.h"

namespace ue5parse::conversion {

ExprId SpirvInterpreter::maskedBits(const ExprId x, const uint32_t mask) {
    ExprId result = zero();
    for (int low = 0; low < 32;) {
        if ((mask >> low & 1u) == 0) {
            ++low;
            continue;
        }
        int high = low;
        while (high < 32 && (mask >> high & 1u) != 0)
            ++high;
        const ExprId shifted = low == 0 ? x : graph.unary(ExprOp::Floor, graph.div(x, graph.constant(std::exp2(static_cast<float>(low)))));
        const ExprId run = graph.binary(ExprOp::Mod, shifted, graph.constant(std::exp2(static_cast<float>(high - low))));
        result = graph.add(result, graph.mul(run, graph.constant(std::exp2(static_cast<float>(low)))));
        low = high;
    }
    return result;
}

ExprId SpirvInterpreter::bit(const ExprId x, const int k, const double bound) {
    const ExprId shifted = k == 0 ? x : graph.unary(ExprOp::Floor, graph.div(x, graph.constant(std::exp2(static_cast<float>(k)))));
    if (bound < std::exp2(k + 1))
        return shifted;
    return graph.binary(ExprOp::Mod, shifted, graph.constant(2.0f));
}

std::optional<double> SpirvInterpreter::nonNegativeBound(const ExprId id) {
    if (const auto found = boundCache.find(id); found != boundCache.end())
        return found->second;
    const ExprNode node = graph.node(id);
    std::optional<double> result;
    const auto both = [&](auto combine) -> std::optional<double> {
        const std::optional<double> a = nonNegativeBound(node.a), b = nonNegativeBound(node.b);
        return a && b ? std::optional(combine(*a, *b)) : std::nullopt;
    };
    if (graph.isBoolean(id))
        result = 1.0;
    else
        switch (node.op) {
        case ExprOp::Constant:
            if (node.value >= 0.0f)
                result = node.value;
            break;
        case ExprOp::Select: {
            const std::optional<double> a = nonNegativeBound(node.b), b = nonNegativeBound(node.c);
            if (a && b)
                result = std::max(*a, *b);
            break;
        }
        case ExprOp::Add:
            result = both([](double a, double b) { return a + b; });
            break;
        case ExprOp::Mul:
            result = both([](double a, double b) { return a * b; });
            break;
        case ExprOp::Max:
            result = both([](double a, double b) { return std::max(a, b); });
            break;
        case ExprOp::Min:
            result = both([](double a, double b) { return std::min(a, b); });
            break;
        case ExprOp::Floor:
        case ExprOp::Trunc:
            result = nonNegativeBound(node.a);
            break;
        case ExprOp::Mod:
            if (graph.isConstant(node.b) && graph.node(node.b).value > 0.0f)
                result = graph.node(node.b).value;
            break;
        default:
            break;
        }
    boundCache.emplace(id, result);
    return result;
}

ExprId SpirvInterpreter::lowBits(const ExprId value) {
    if (const auto found = lowBitsCache.find(value); found != lowBitsCache.end())
        return found->second;
    const ExprNode node = graph.node(value);
    const ExprId modulus = graph.constant(hashModulus);
    ExprId result;
    if (node.op == ExprOp::Constant)
        result = graph.constant(node.value - hashModulus * std::floor(node.value / hashModulus));
    else if (node.op == ExprOp::Add || node.op == ExprOp::Sub || node.op == ExprOp::Mul)
        result = graph.binary(ExprOp::Mod, graph.binary(node.op, lowBits(node.a), lowBits(node.b)), modulus);
    else
        result = graph.binary(ExprOp::Mod, value, modulus);
    lowBitsCache.emplace(value, result);
    return result;
}

}
