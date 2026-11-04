#include <algorithm>
#include <array>
#include <numeric>

#include "Conversion/Spirv/ProgramWriter.h"

namespace ue5parse::conversion {

std::optional<ProgramWriter::Clamp> ProgramWriter::clamp(const ExprId id) const {
    const ExprNode& node = graph.node(id);
    if (node.op != ExprOp::Min)
        return std::nullopt;
    for (const auto& [inner, high] : {std::pair{node.a, node.b}, std::pair{node.b, node.a}}) {
        const ExprNode& bounded = graph.node(inner);
        if (bounded.op != ExprOp::Max)
            continue;
        if (graph.isConstant(bounded.a))
            return Clamp{bounded.b, bounded.a, high};
        return Clamp{bounded.a, bounded.b, high};
    }
    return std::nullopt;
}

std::optional<uint32_t> ProgramWriter::smoothstep(const ExprId id) {
    const ExprNode& node = graph.node(id);
    for (const auto [square, ramp] : {std::pair{node.a, node.b}, std::pair{node.b, node.a}}) {
        const ExprNode& squared = graph.node(square);
        const ExprNode& falling = graph.node(ramp);
        if (squared.op != ExprOp::Mul || squared.a != squared.b || falling.op != ExprOp::Sub || !graph.isConstant(falling.a, 3.0f))
            continue;
        const ExprId t = squared.a;
        const ExprNode& twice = graph.node(falling.b);
        if (twice.op != ExprOp::Mul || !((twice.a == t && graph.isConstant(twice.b, 2.0f)) || (twice.b == t && graph.isConstant(twice.a, 2.0f))))
            continue;
        const std::optional<Clamp> clamped = clamp(t);
        if (!clamped || !graph.isConstant(clamped->low, 0.0f) || !graph.isConstant(clamped->high, 1.0f))
            continue;
        const ExprNode& ratio = graph.node(clamped->value);
        if (ratio.op == ExprOp::Div) {
            const ExprNode& offset = graph.node(ratio.a);
            const ExprNode& range = graph.node(ratio.b);
            if (offset.op == ExprOp::Sub && range.op == ExprOp::Sub && offset.b == range.b)
                return instruction(ProgramOp::Smoothstep, 1, 0, emit(offset.a), emit(offset.b), emit(range.a));
        }
        return instruction(ProgramOp::Smoothstep, 1, 0, emit(clamped->value), emit(clamped->low), emit(clamped->high));
    }
    return std::nullopt;
}

std::optional<ProgramWriter::Lanes> ProgramWriter::sumTerms(const ExprId id) const {
    Lanes terms;
    for (std::vector<ExprId> stack{id}; !stack.empty();) {
        const ExprId term = stack.back();
        stack.pop_back();
        const ExprNode& node = graph.node(term);
        if (node.op == ExprOp::Add) {
            stack.push_back(node.b);
            stack.push_back(node.a);
            continue;
        }
        if (terms.size() == 3)
            return std::nullopt;
        terms.push_back(term);
    }
    return terms;
}

std::optional<ProgramWriter::Lanes> ProgramWriter::squares(const ExprId id) const {
    auto terms = sumTerms(id);
    if (!terms || terms->size() < 2)
        return std::nullopt;
    for (ExprId& term : *terms) {
        const ExprNode& product = graph.node(term);
        if (product.op != ExprOp::Mul || product.a != product.b)
            return std::nullopt;
        term = product.a;
    }
    return terms;
}

ProgramWriter::Lanes ProgramWriter::factors(const ExprId id) const {
    Lanes result;
    for (std::vector<ExprId> stack{id}; !stack.empty();) {
        const ExprId value = stack.back();
        stack.pop_back();
        const ExprNode& node = graph.node(value);
        if (node.op != ExprOp::Mul) {
            result.push_back(value);
            continue;
        }
        stack.push_back(node.b);
        stack.push_back(node.a);
    }
    return result;
}

std::optional<ProgramWriter::DotProduct> ProgramWriter::dotProduct(const ExprId id) {
    const auto terms = sumTerms(id);
    if (!terms || terms->size() < 2 || !std::ranges::all_of(*terms, [&](const ExprId term) { return graph.node(term).op == ExprOp::Mul; }))
        return std::nullopt;
    std::vector<std::pair<ExprId, ExprId>> products;
    for (const ExprId term : *terms)
        products.emplace_back(graph.node(term).a, graph.node(term).b);
    if (auto lanes = pairLanes(products))
        return DotProduct{std::move(lanes->first), std::move(lanes->second), {}};

    std::vector<Lanes> termFactors;
    for (const ExprId term : *terms) {
        termFactors.push_back(factors(term));
        std::ranges::sort(termFactors.back());
    }
    Lanes shared = termFactors[0];
    for (const Lanes& other : termFactors) {
        Lanes common;
        std::ranges::set_intersection(shared, other, std::back_inserter(common));
        shared = std::move(common);
    }
    if (shared.empty())
        return std::nullopt;
    products.clear();
    for (const Lanes& other : termFactors) {
        Lanes rest;
        std::ranges::set_difference(other, shared, std::back_inserter(rest));
        if (rest.size() != 2)
            return std::nullopt;
        products.emplace_back(rest[0], rest[1]);
    }
    auto lanes = pairLanes(products);
    if (!lanes)
        return std::nullopt;
    return DotProduct{std::move(lanes->first), std::move(lanes->second), std::move(shared)};
}

std::optional<std::pair<ProgramWriter::Lanes, ProgramWriter::Lanes>> ProgramWriter::pairLanes(const std::vector<std::pair<ExprId, ExprId>>& pairs) {
    const auto vectorizable = [](const Shape shape) { return shape == Shape::Leaf || shape == Shape::Operation || shape == Shape::Constant; };
    std::vector<size_t> order(pairs.size());
    std::iota(order.begin(), order.end(), 0);
    do {
        for (uint32_t flips = 0; flips < (1u << pairs.size()); ++flips) {
            Lanes a, b;
            for (size_t i = 0; i < pairs.size(); ++i) {
                const auto [first, second] = pairs[order[i]];
                const bool flip = (flips >> i) & 1u;
                a.push_back(flip ? second : first);
                b.push_back(flip ? first : second);
            }
            const Shape first = shape(a), second = shape(b);
            if (vectorizable(first) && vectorizable(second) && (first != Shape::Constant || second != Shape::Constant))
                return std::pair{a, b};
        }
    } while (std::next_permutation(order.begin(), order.end()));
    return std::nullopt;
}

std::optional<std::pair<ProgramWriter::Lanes, ProgramWriter::Lanes>> ProgramWriter::crossProduct(const Lanes& lanes) {
    if (lanes.size() != 3)
        return std::nullopt;
    std::array<const ExprNode*, 3> differences{};
    for (size_t i = 0; i < 3; ++i) {
        differences[i] = &graph.node(lanes[i]);
        if (differences[i]->op != ExprOp::Sub || graph.node(differences[i]->a).op != ExprOp::Mul || graph.node(differences[i]->b).op != ExprOp::Mul)
            return std::nullopt;
    }
    const auto factors = [&](const size_t lane, const bool first) {
        const ExprNode& product = graph.node(first ? differences[lane]->a : differences[lane]->b);
        return std::pair{product.a, product.b};
    };
    const auto other = [](const std::pair<ExprId, ExprId>& product, const ExprId known) -> std::optional<ExprId> {
        if (product.first == known)
            return product.second;
        if (product.second == known)
            return product.first;
        return std::nullopt;
    };
    for (uint32_t flips = 0; flips < 4; ++flips) {
        const auto [p0, p1] = factors(0, true);
        const auto [q0, q1] = factors(0, false);
        Lanes a(3), b(3);
        a[1] = flips & 1u ? p1 : p0;
        b[2] = flips & 1u ? p0 : p1;
        a[2] = flips & 2u ? q1 : q0;
        b[1] = flips & 2u ? q0 : q1;
        const auto b0 = other(factors(1, true), a[2]);
        const auto a0 = other(factors(1, false), b[2]);
        if (!b0 || !a0)
            continue;
        a[0] = *a0;
        b[0] = *b0;
        if (other(factors(2, true), a[0]) != b[1] || other(factors(2, false), a[1]) != b[0])
            continue;
        const Shape first = shape(a), second = shape(b);
        if (first != Shape::None && first != Shape::Uniform && second != Shape::None && second != Shape::Uniform)
            return std::pair{a, b};
    }
    return std::nullopt;
}

std::optional<ProgramWriter::Lanes> ProgramWriter::normalized(const Lanes& lanes) const {
    const ExprNode& first = graph.node(lanes[0]);
    if (lanes.size() < 2 || first.op != ExprOp::Mul)
        return std::nullopt;
    for (const ExprId inverse : {first.a, first.b}) {
        const ExprNode& reciprocal = graph.node(inverse);
        if (reciprocal.op != ExprOp::Div || !graph.isConstant(reciprocal.a, 1.0f) || graph.node(reciprocal.b).op != ExprOp::Sqrt)
            continue;
        Lanes values;
        for (const ExprId lane : lanes) {
            const ExprNode& node = graph.node(lane);
            if (node.op != ExprOp::Mul)
                return std::nullopt;
            if (node.a == inverse)
                values.push_back(node.b);
            else if (node.b == inverse)
                values.push_back(node.a);
            else
                break;
        }
        auto squared = squares(graph.node(reciprocal.b).a);
        if (values.size() != lanes.size() || !squared)
            continue;
        Lanes sorted = values;
        std::ranges::sort(sorted);
        std::ranges::sort(*squared);
        if (*squared == sorted)
            return values;
    }
    return std::nullopt;
}

}
