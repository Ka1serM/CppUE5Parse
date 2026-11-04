#include "Conversion/Spirv/ExpressionGraph.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <utility>

namespace ue5parse::conversion {
namespace {

constexpr uint32_t booleanMark = 1;

bool commutative(const ExprOp op) {
    switch (op) {
    case ExprOp::Add:
    case ExprOp::Mul:
    case ExprOp::Min:
    case ExprOp::Max:
    case ExprOp::Equal:
    case ExprOp::NotEqual:
    case ExprOp::And:
    case ExprOp::Or:
        return true;
    default:
        return false;
    }
}

bool condition(const ExprOp op) {
    switch (op) {
    case ExprOp::Less:
    case ExprOp::LessEqual:
    case ExprOp::Equal:
    case ExprOp::NotEqual:
    case ExprOp::And:
    case ExprOp::Or:
    case ExprOp::Not:
        return true;
    default:
        return false;
    }
}

bool isUnary(const ExprOp op) {
    return (op >= ExprOp::Abs && op <= ExprOp::Random) || op == ExprOp::Not;
}

struct Operands {
    std::array<ExprId, 3> ids{};
    size_t count = 0;

    const ExprId* begin() const { return ids.data(); }
    const ExprId* end() const { return ids.data() + count; }
    bool empty() const { return count == 0; }
};

Operands operands(const ExprNode& node) {
    switch (node.op) {
    case ExprOp::Constant:
    case ExprOp::Parameter:
    case ExprOp::Geometry:
    case ExprOp::GeometryDx:
    case ExprOp::GeometryDy:
    case ExprOp::Time:
    case ExprOp::Frame:
    case ExprOp::Unresolved:
        return {};
    case ExprOp::Texture:
        return {{node.a, node.b}, 2};
    case ExprOp::Select:
        return {{node.a, node.b, node.c}, 3};
    default:
        return {{node.a, node.b}, isUnary(node.op) ? 1u : 2u};
    }
}

bool perPixel(const GeometryAttribute attribute) {
    return attribute <= GeometryAttribute::VertexColor;
}

float evaluate(const ExprOp op, const float a, const float b) {
    switch (op) {
    case ExprOp::Add:
        return a + b;
    case ExprOp::Sub:
        return a - b;
    case ExprOp::Mul:
        return a * b;
    case ExprOp::Div:
        return a / b;
    case ExprOp::Min:
        return std::fmin(a, b);
    case ExprOp::Max:
        return std::fmax(a, b);
    case ExprOp::Pow:
        return std::pow(a, b);
    case ExprOp::Atan2:
        return std::atan2(a, b);
    case ExprOp::Mod:
        return a - b * std::floor(a / b);
    case ExprOp::Abs:
        return std::fabs(a);
    case ExprOp::Floor:
        return std::floor(a);
    case ExprOp::Ceil:
        return std::ceil(a);
    case ExprOp::Fract:
        return a - std::floor(a);
    case ExprOp::Round:
        return std::nearbyint(a);
    case ExprOp::Trunc:
        return std::trunc(a);
    case ExprOp::Sign:
        return a > 0.0f ? 1.0f : a < 0.0f ? -1.0f : 0.0f;
    case ExprOp::Sqrt:
        return std::sqrt(a);
    case ExprOp::Exp2:
        return std::exp2(a);
    case ExprOp::Log2:
        return std::log2(a);
    case ExprOp::Sin:
        return std::sin(a);
    case ExprOp::Cos:
        return std::cos(a);
    case ExprOp::Tan:
        return std::tan(a);
    case ExprOp::Asin:
        return std::asin(a);
    case ExprOp::Acos:
        return std::acos(a);
    case ExprOp::Atan:
        return std::atan(a);
    case ExprOp::Less:
        return a < b;
    case ExprOp::LessEqual:
        return a <= b;
    case ExprOp::Equal:
        return a == b;
    case ExprOp::NotEqual:
        return a != b;
    case ExprOp::And:
        return a != 0.0f && b != 0.0f;
    case ExprOp::Or:
        return a != 0.0f || b != 0.0f;
    case ExprOp::Not:
        return a == 0.0f;
    default:
        return 0.0f;
    }
}
}

size_t ExpressionGraph::ExprNodeHash::operator()(const ExprNode& node) const {
    size_t hash = std::hash<uint32_t>{}(static_cast<uint32_t>(node.op));
    const auto combine = [&](const size_t value) { hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2); };
    combine(node.index);
    combine(node.component);
    combine(std::hash<float>{}(node.value));
    combine(node.a);
    combine(node.b);
    combine(node.c);
    return hash;
}

ExpressionGraph::ExpressionGraph() {
    falseId = intern({.op = ExprOp::Constant, .component = booleanMark, .value = 0.0f});
    trueId = intern({.op = ExprOp::Constant, .component = booleanMark, .value = 1.0f});
}

ExprId ExpressionGraph::intern(const ExprNode& node) {
    if (const auto found = index.find(node); found != index.end())
        return found->second;
    const auto id = static_cast<ExprId>(nodes.size());
    nodes.push_back(node);
    index.emplace(node, id);
    return id;
}

ExprId ExpressionGraph::constant(const float value) {
    return intern({.op = ExprOp::Constant, .value = std::isnan(value) || value == 0.0f ? 0.0f : value});
}

ExprId ExpressionGraph::parameter(const uint32_t parameter, const uint32_t component) {
    return intern({.op = ExprOp::Parameter, .index = parameter, .component = component});
}

ExprId ExpressionGraph::texture(const uint32_t slot, const uint32_t component, const ExprId u, const ExprId v) {
    return intern({.op = ExprOp::Texture, .index = slot, .component = component, .a = u, .b = v});
}

ExprId ExpressionGraph::geometry(const GeometryAttribute attribute, const uint32_t component) {
    return intern({.op = ExprOp::Geometry, .index = static_cast<uint32_t>(attribute), .component = component});
}

ExprId ExpressionGraph::geometryDerivative(const GeometryAttribute attribute, const uint32_t component, const ScreenAxis axis) {
    return intern({.op = axis == ScreenAxis::X ? ExprOp::GeometryDx : ExprOp::GeometryDy, .index = static_cast<uint32_t>(attribute), .component = component});
}

ExprId ExpressionGraph::time() {
    return intern({.op = ExprOp::Time});
}

ExprId ExpressionGraph::frame() {
    return intern({.op = ExprOp::Frame});
}

ExprId ExpressionGraph::unresolved(const std::string& name) {
    auto [entry, inserted] = unresolvedIndices.try_emplace(name, static_cast<uint32_t>(unresolvedNames.size()));
    if (inserted)
        unresolvedNames.push_back(name);
    return intern({.op = ExprOp::Unresolved, .index = entry->second});
}

bool ExpressionGraph::isBoolean(const ExprId id) const {
    const ExprNode& node = nodes[id];
    return condition(node.op) || (node.op == ExprOp::Constant && node.component == booleanMark);
}

void ExpressionGraph::annotate(const std::span<const ExprId> outputs, SemanticExpression expression) {
    if (outputs.empty())
        return;
    std::unordered_map<ExprId, bool> unique;
    for (const ExprId output : outputs)
        if (isConstant(output) || isBoolean(output) || semanticOutputs.contains(output) || !unique.emplace(output, true).second)
            return;
    for (const std::vector<ExprId>* operands : {&expression.a, &expression.b, &expression.c})
        for (const ExprId operand : *operands)
            if (unique.contains(operand))
                return;
    const uint32_t index = static_cast<uint32_t>(semanticExpressions.size());
    expression.outputCount = static_cast<uint32_t>(outputs.size());
    semanticExpressions.push_back(std::move(expression));
    for (size_t component = 0; component < outputs.size(); ++component)
        semanticOutputs.emplace(outputs[component], std::pair{index, static_cast<uint32_t>(component)});
}

std::optional<std::pair<uint32_t, uint32_t>> ExpressionGraph::semanticOutput(const ExprId id) const {
    const auto found = semanticOutputs.find(id);
    return found == semanticOutputs.end() ? std::nullopt : std::optional(found->second);
}

ExprId ExpressionGraph::unary(const ExprOp op, const ExprId a) {
    return fold(op, a, 0, 0);
}

ExprId ExpressionGraph::binary(const ExprOp op, ExprId a, ExprId b) {
    if (commutative(op) && a > b)
        std::swap(a, b);
    return fold(op, a, b, 0);
}

ExprId ExpressionGraph::select(const ExprId condition, const ExprId a, const ExprId b) {
    return fold(ExprOp::Select, condition, a, b);
}

ExprId ExpressionGraph::screenDerivative(const ExprId value, const ScreenAxis axis) {
    return sub(shifted(value, axis), value);
}

ExprId ExpressionGraph::shifted(const ExprId value, const ScreenAxis axis) {
    auto& done = shiftedCache[static_cast<size_t>(axis)];
    std::vector<ExprId> stack{value};
    while (!stack.empty()) {
        const ExprId id = stack.back();
        if (done.contains(id)) {
            stack.pop_back();
            continue;
        }
        const ExprNode node = nodes[id];
        bool ready = true;
        for (const ExprId operand : operands(node))
            if (!done.contains(operand)) {
                stack.push_back(operand);
                ready = false;
            }
        if (!ready)
            continue;
        stack.pop_back();
        const auto moved = [&](const ExprId operand) { return done.at(operand); };
        ExprId result = id;
        switch (node.op) {
        case ExprOp::Geometry: {
            const auto attribute = static_cast<GeometryAttribute>(node.index);
            if (perPixel(attribute))
                result = add(id, geometryDerivative(attribute, node.component, axis));
            break;
        }
        case ExprOp::Texture:
            result = texture(node.index, node.component, moved(node.a), moved(node.b));
            break;
        case ExprOp::Select:
            result = select(moved(node.a), moved(node.b), moved(node.c));
            break;
        default:
            if (!operands(node).empty())
                result = isUnary(node.op) ? unary(node.op, moved(node.a)) : binary(node.op, moved(node.a), moved(node.b));
        }
        done.emplace(id, result);
    }
    return done.at(value);
}

ExprId ExpressionGraph::fold(const ExprOp op, const ExprId a, const ExprId b, const ExprId c) {
    const bool unaryOp = isUnary(op);
    if (op == ExprOp::Select) {
        if (a == trueId)
            return b;
        if (a == falseId)
            return c;
        if (b == c)
            return b;
        if (nodes[a].op == ExprOp::Not)
            return fold(ExprOp::Select, nodes[a].a, c, b);
        return intern({.op = op, .a = a, .b = b, .c = c});
    }
    if (op != ExprOp::Random && isConstant(a) && (unaryOp || isConstant(b))) {
        const float result = evaluate(op, nodes[a].value, unaryOp ? 0.0f : nodes[b].value);
        if (condition(op))
            return boolean(result != 0.0f);
        if (!std::isfinite(result))
            return intern({.op = op, .a = a, .b = unaryOp ? 0 : b});
        return constant(result);
    }
    switch (op) {
    case ExprOp::Add:
        if (isConstant(a, 0.0f))
            return b;
        if (isConstant(b, 0.0f))
            return a;
        if (const std::optional<ExprId> folded = reassociate(op, a, b))
            return *folded;
        break;
    case ExprOp::Sub:
        if (isConstant(b, 0.0f))
            return a;
        if (isConstant(b) && !isBoolean(b))
            return binary(ExprOp::Add, a, constant(-nodes[b].value));
        if (a == b)
            return constant(0.0f);
        if (nodes[a].op == ExprOp::Add && (nodes[a].a == b || nodes[a].b == b))
            return nodes[a].a == b ? nodes[a].b : nodes[a].a;
        break;
    case ExprOp::Mul:
        if (isConstant(a, 0.0f) || isConstant(b, 0.0f))
            return constant(0.0f);
        if (isConstant(a, 1.0f))
            return b;
        if (isConstant(b, 1.0f))
            return a;
        if (const std::optional<ExprId> folded = reassociate(op, a, b))
            return *folded;
        break;
    case ExprOp::Div:
        if (isConstant(b, 1.0f))
            return a;
        if (isConstant(a, 0.0f))
            return constant(0.0f);
        break;
    case ExprOp::Pow:
        if (isConstant(b, 1.0f))
            return a;
        if (isConstant(b, 0.0f))
            return constant(1.0f);
        break;
    case ExprOp::Min:
    case ExprOp::Max:
        if (a == b)
            return a;
        break;
    case ExprOp::Less:
    case ExprOp::NotEqual:
        if (a == b)
            return falseId;
        break;
    case ExprOp::LessEqual:
    case ExprOp::Equal:
        if (a == b)
            return trueId;
        break;
    case ExprOp::And:
        if (a == falseId || b == falseId)
            return falseId;
        if (a == trueId)
            return b;
        if (b == trueId)
            return a;
        if (a == b)
            return a;
        break;
    case ExprOp::Or:
        if (a == trueId || b == trueId)
            return trueId;
        if (a == falseId)
            return b;
        if (b == falseId)
            return a;
        if (a == b)
            return a;
        break;
    case ExprOp::Not:
        if (nodes[a].op == ExprOp::Not)
            return nodes[a].a;
        break;
    default:
        break;
    }
    return intern({.op = op, .a = a, .b = unaryOp ? 0 : b});
}

std::optional<ExprId> ExpressionGraph::reassociate(const ExprOp op, const ExprId a, const ExprId b) {
    const bool constantFirst = isConstant(a);
    if (!constantFirst && !isConstant(b))
        return std::nullopt;
    const ExprId outer = constantFirst ? a : b;
    const ExprNode inner = nodes[constantFirst ? b : a];
    if (inner.op != op || isBoolean(outer))
        return std::nullopt;
    const bool innerConstantFirst = isConstant(inner.a);
    if (!innerConstantFirst && !isConstant(inner.b))
        return std::nullopt;
    const float combined = evaluate(op, nodes[outer].value, nodes[innerConstantFirst ? inner.a : inner.b].value);
    if (!std::isfinite(combined))
        return std::nullopt;
    return binary(op, constant(combined), innerConstantFirst ? inner.b : inner.a);
}

std::vector<ExprId> ExpressionGraph::unresolvedInputs(const ExprId root) const {
    std::vector<ExprId> result;
    std::vector<bool> visited(nodes.size(), false);
    std::vector<ExprId> stack{root};
    while (!stack.empty()) {
        const ExprId id = stack.back();
        stack.pop_back();
        if (visited[id])
            continue;
        visited[id] = true;
        const ExprNode& node = nodes[id];
        if (node.op == ExprOp::Unresolved)
            result.push_back(id);
        for (const ExprId operand : operands(node))
            stack.push_back(operand);
    }
    return result;
}

std::optional<ExprId> ExpressionGraph::coefficient(const ExprId value, const ExprId leaf) {
    const auto depends = [&](const ExprId id) { return std::ranges::contains(unresolvedInputs(id), leaf); };
    if (value == leaf)
        return constant(1.0f);
    if (!depends(value))
        return constant(0.0f);
    const ExprNode node = nodes[value];
    const auto both = [&](const auto combine) -> std::optional<ExprId> {
        const std::optional<ExprId> a = coefficient(node.a, leaf);
        const std::optional<ExprId> b = coefficient(node.b, leaf);
        if (!a || !b)
            return std::nullopt;
        return combine(*a, *b);
    };
    switch (node.op) {
    case ExprOp::Add:
        return both([&](const ExprId a, const ExprId b) { return add(a, b); });
    case ExprOp::Sub:
        return both([&](const ExprId a, const ExprId b) { return sub(a, b); });
    case ExprOp::Mul:
        if (!depends(node.b))
            return coefficient(node.a, leaf).transform([&](const ExprId a) { return mul(a, node.b); });
        if (!depends(node.a))
            return coefficient(node.b, leaf).transform([&](const ExprId b) { return mul(node.a, b); });
        return std::nullopt;
    case ExprOp::Div:
        if (!depends(node.b))
            return coefficient(node.a, leaf).transform([&](const ExprId a) { return div(a, node.b); });
        return std::nullopt;
    case ExprOp::Select: {
        if (depends(node.a))
            return std::nullopt;
        const std::optional<ExprId> b = coefficient(node.b, leaf);
        const std::optional<ExprId> c = coefficient(node.c, leaf);
        if (!b || !c)
            return std::nullopt;
        return select(node.a, *b, *c);
    }
    default:
        return std::nullopt;
    }
}

void ExpressionGraph::fail(std::string message) {
    if (!failure)
        failure = std::move(message);
}

}
