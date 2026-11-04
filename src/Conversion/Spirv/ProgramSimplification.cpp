#include <algorithm>
#include <array>

#include "Conversion/Spirv/ProgramWriter.h"

namespace ue5parse::conversion {

bool ProgramWriter::isGeometry(const uint32_t id, const GeometryAttribute attribute) const {
    const ProgramInstruction& instruction = program.instructions[id];
    return instruction.op == ProgramOp::Geometry && instruction.index == static_cast<uint16_t>(attribute);
}

bool ProgramWriter::isConstant(const uint32_t id, const float value) const {
    const ProgramInstruction& instruction = program.instructions[id];
    return instruction.op == ProgramOp::Constant && instruction.width == 1 && program.constants[instruction.index] == value;
}

std::optional<uint32_t> ProgramWriter::simplify(const ProgramOp op, const uint32_t width, const uint32_t a, const uint32_t b, const uint32_t c) {
    const auto at = [&](const uint32_t id) -> const ProgramInstruction& { return program.instructions[id]; };
    if (op == ProgramOp::Combine) {
        const std::array<uint32_t, 3> components{a, b, c};
        const uint32_t vector = at(a).a;
        for (uint32_t i = 0; i < width; ++i)
            if (at(components[i]).op != ProgramOp::Extract || at(components[i]).index != i || at(components[i]).a != vector)
                return std::nullopt;
        return vector;
    }
    if (op == ProgramOp::Select && width == 1 && isConstant(b, 1.0f) && isConstant(c, 0.0f))
        return a;
    if (op == ProgramOp::Select && width == 1 && isConstant(b, 0.0f) && isConstant(c, 1.0f)) {
        switch (at(a).op) {
        case ProgramOp::Less:
            return instruction(ProgramOp::LessEqual, 1, 0, at(a).b, at(a).a);
        case ProgramOp::LessEqual:
            return instruction(ProgramOp::Less, 1, 0, at(a).b, at(a).a);
        case ProgramOp::Equal:
            return instruction(ProgramOp::NotEqual, 1, 0, at(a).a, at(a).b);
        case ProgramOp::NotEqual:
            return instruction(ProgramOp::Equal, 1, 0, at(a).a, at(a).b);
        default:
            return instruction(ProgramOp::Sub, 1, 0, constant(1.0f), a);
        }
    }
    if (op == ProgramOp::Clamp && width == 1 && isConstant(b, 0.0f) && isConstant(c, 1.0f)
        && (at(a).op == ProgramOp::Less || at(a).op == ProgramOp::LessEqual || at(a).op == ProgramOp::Equal || at(a).op == ProgramOp::NotEqual))
        return a;
    if (op == ProgramOp::Select && at(a).op == ProgramOp::LessEqual)
        return instruction(ProgramOp::Select, width, 0, instruction(ProgramOp::Less, 1, 0, at(a).b, at(a).a), c, b);
    if (op == ProgramOp::Select && at(a).op == ProgramOp::NotEqual)
        return instruction(ProgramOp::Select, width, 0, instruction(ProgramOp::Equal, 1, 0, at(a).a, at(a).b), c, b);
    if (op == ProgramOp::Mul && width == 3) {
        for (const auto [cross, sign] : {std::pair{a, b}, std::pair{b, a}}) {
            if (at(cross).op != ProgramOp::Cross || !isGeometry(at(cross).a, GeometryAttribute::Normal) || !isGeometry(at(cross).b, GeometryAttribute::Tangent)
                || at(sign).op != ProgramOp::Sign)
                continue;
            const ProgramInstruction& dot = at(at(sign).a);
            if (dot.op != ProgramOp::Dot
                || !((dot.a == cross && isGeometry(dot.b, GeometryAttribute::Bitangent))
                     || (dot.b == cross && isGeometry(dot.a, GeometryAttribute::Bitangent))))
                continue;
            return instruction(ProgramOp::Geometry, 3, static_cast<uint32_t>(GeometryAttribute::Bitangent));
        }
    }
    if (op == ProgramOp::Normalize && width == 3)
        return tangentToWorld(a);
    return std::nullopt;
}

std::optional<uint32_t> ProgramWriter::tangentToWorld(const uint32_t sum) {
    std::vector<uint32_t> terms;
    for (std::vector<uint32_t> pending{sum}; !pending.empty();) {
        const uint32_t id = pending.back();
        pending.pop_back();
        const ProgramInstruction& term = program.instructions[id];
        if (term.op == ProgramOp::Add && term.width == 3) {
            pending.push_back(term.a);
            pending.push_back(term.b);
            continue;
        }
        if (terms.size() == 3)
            return std::nullopt;
        terms.push_back(id);
    }
    std::array<std::optional<uint32_t>, 3> factors;
    constexpr std::array axes{GeometryAttribute::Tangent, GeometryAttribute::Bitangent, GeometryAttribute::Normal};
    for (const uint32_t id : terms) {
        const ProgramInstruction& term = program.instructions[id];
        if (term.op != ProgramOp::Mul)
            return std::nullopt;
        for (size_t axis = 0; axis < 3; ++axis)
            for (const auto [basis, factor] : {std::pair{term.a, term.b}, std::pair{term.b, term.a}})
                if (isGeometry(basis, axes[axis]) && width(factor) == 1 && !factors[axis])
                    factors[axis] = factor;
    }
    if (!factors[0] || !factors[1] || !factors[2])
        return std::nullopt;
    const std::array<uint32_t, 3> vector = withoutScale({*factors[0], *factors[1], *factors[2]});
    return instruction(ProgramOp::TangentToWorld, 3, 0, instruction(ProgramOp::Combine, 3, 0, vector[0], vector[1], vector[2]));
}

std::array<uint32_t, 3> ProgramWriter::withoutScale(const std::array<uint32_t, 3> factors) const {
    const auto at = [&](const uint32_t id) -> const ProgramInstruction& { return program.instructions[id]; };
    if (at(factors[0]).op != ProgramOp::Mul)
        return factors;
    for (const uint32_t scale : {at(factors[0]).a, at(factors[0]).b}) {
        const ProgramInstruction& divide = at(scale);
        if (divide.op != ProgramOp::Div || at(divide.a).op != ProgramOp::Constant || at(divide.a).width != 1 || program.constants[at(divide.a).index] <= 0.0f
            || at(divide.b).op != ProgramOp::Length)
            continue;
        std::array<uint32_t, 3> unscaled{};
        bool scaled = true;
        for (size_t i = 0; i < 3 && scaled; ++i) {
            const ProgramInstruction& product = at(factors[i]);
            scaled = product.op == ProgramOp::Mul && (product.a == scale || product.b == scale);
            unscaled[i] = product.a == scale ? product.b : product.a;
        }
        if (scaled)
            return unscaled;
    }
    return factors;
}

}
