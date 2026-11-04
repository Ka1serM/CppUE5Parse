#include "Conversion/Spirv/SpirvInterpreter.h"

namespace ue5parse::conversion {

void SpirvInterpreter::execute(const Instruction& instruction, const ExprId path) {
    const auto& o = instruction.operands;
    switch (instruction.op) {
    case spv::OpStore:
    case spv::OpNop:
    case spv::OpLine:
    case spv::OpNoLine:
    case spv::OpDemoteToHelperInvocation:
    case spv::OpImageWrite:
    case spv::OpAtomicStore:
    case spv::OpMemoryBarrier:
    case spv::OpControlBarrier:
        break;
    default:
        valueTypes[o[1]] = o[0];
    }
    switch (instruction.op) {
    case spv::OpNop:
    case spv::OpLine:
    case spv::OpNoLine:
        return;
    case spv::OpImageWrite:
    case spv::OpAtomicStore:
    case spv::OpMemoryBarrier:
    case spv::OpControlBarrier:
        return;
    case spv::OpImageTexelPointer:
    case spv::OpImageRead:
    case spv::OpAtomicLoad:
    case spv::OpAtomicExchange:
    case spv::OpAtomicCompareExchange:
    case spv::OpAtomicIIncrement:
    case spv::OpAtomicIDecrement:
    case spv::OpAtomicIAdd:
    case spv::OpAtomicISub:
    case spv::OpAtomicSMin:
    case spv::OpAtomicUMin:
    case spv::OpAtomicSMax:
    case spv::OpAtomicUMax:
    case spv::OpAtomicAnd:
    case spv::OpAtomicOr:
    case spv::OpAtomicXor:
        values[o[1]] = std::vector<Scalar>(componentCount(o[0]), Scalar{zero(), 0u});
        return;
    case spv::OpVariable: {
        const Variable variable{types.at(o[0]).element, spv::StorageClassFunction, o.size() > 3 ? std::optional(o[3]) : std::nullopt};
        variables[o[1]] = variable;
        memory[o[1]] = initialValue(variable);
        values[o[1]] = Pointer{o[1], variable.type, 0, 0};
        return;
    }
    case spv::OpAccessChain:
    case spv::OpInBoundsAccessChain:
        values[o[1]] = accessChain(o[2], o.subspan(3));
        return;
    case spv::OpLoad:
        values[o[1]] = load(o[2]);
        return;
    case spv::OpStore: {
        const std::vector<Scalar>& value = scalars(o[1]);
        if (!graph.isFailed())
            store(o[0], value);
        return;
    }
    case spv::OpDemoteToHelperInvocation:
        discard = graph.logicalOr(discard, path);
        return;
    case spv::OpSampledImage:
    case spv::OpImage:
        if (const std::optional<Handle> handle = get<Handle>(o[2]))
            values[o[1]] = *handle;
        return;
    case spv::OpImageSampleImplicitLod:
    case spv::OpImageSampleExplicitLod: {
        const std::optional<Handle> handle = get<Handle>(o[2]);
        std::vector<ExprId> coordinate;
        for (const Scalar& scalar : scalars(o[3]))
            coordinate.push_back(scalar.expr);
        if (!handle || coordinate.empty())
            return graph.fail("A texture sample has no texture or coordinate.");
        const auto result = inputs.sample(handle->binding, coordinate);
        std::vector<Scalar> components;
        components.reserve(result.size());
        for (const ExprId component : result)
            components.push_back(floating(component));
        components.resize(componentCount(o[0]));
        values[o[1]] = std::move(components);
        return;
    }
    case spv::OpImageFetch: {
        const std::optional<Handle> handle = get<Handle>(o[2]);
        const ExprId index = scalar(o[3]).expr;
        if (!handle)
            return;
        const auto result = inputs.fetch(handle->binding, index);
        std::vector<Scalar> components;
        components.reserve(result.size());
        for (const ExprId component : result)
            components.push_back(floating(component));
        components.resize(componentCount(o[0]));
        values[o[1]] = std::move(components);
        return;
    }
    case spv::OpImageGather:
        gather(o);
        return;
    case spv::OpCopyObject: {
        const auto found = values.find(o[2]);
        if (found == values.end())
            return graph.fail("The SPIR-V value %" + std::to_string(o[2]) + " is not available.");
        values[o[1]] = found->second;
        return;
    }
    case spv::OpCompositeConstruct: {
        std::vector<Scalar> result;
        for (const uint32_t part : o.subspan(2))
            append(result, scalars(part));
        values[o[1]] = std::move(result);
        return;
    }
    case spv::OpCompositeExtract: {
        const auto [first, count] = compositeRange(o[2], o.subspan(3));
        const std::vector<Scalar>& source = scalars(o[2]);
        if (first + count > source.size())
            return graph.fail("A composite extract reads past its composite.");
        values[o[1]] = std::vector<Scalar>(source.begin() + first, source.begin() + first + count);
        return;
    }
    case spv::OpCompositeInsert: {
        std::vector<Scalar> result = scalars(o[3]);
        const auto [first, count] = compositeRange(o[3], o.subspan(4));
        const std::vector<Scalar>& part = scalars(o[2]);
        if (first + count > result.size() || count > part.size())
            return graph.fail("A composite insert writes past its composite.");
        std::copy(part.begin(), part.begin() + count, result.begin() + first);
        values[o[1]] = std::move(result);
        return;
    }
    case spv::OpVectorShuffle: {
        std::vector<Scalar> both = scalars(o[2]);
        append(both, scalars(o[3]));
        std::vector<Scalar> result;
        for (const uint32_t index : o.subspan(4)) {
            if (index != 0xffffffffu && index >= both.size())
                return graph.fail("A vector shuffle reads past its vectors.");
            result.push_back(index == 0xffffffffu ? Scalar{zero(), 0u} : both[index]);
        }
        values[o[1]] = std::move(result);
        return;
    }
    case spv::OpSelect: {
        const std::vector<Scalar>& condition = scalars(o[2]);
        const std::vector<Scalar>& a = scalars(o[3]);
        const std::vector<Scalar>& b = scalars(o[4]);
        if (b.size() != a.size() || (condition.size() != 1 && condition.size() != a.size()))
            return graph.fail("A select has operands of different sizes.");
        std::vector<Scalar> result(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            const ExprId c = condition[condition.size() == 1 ? 0 : i].expr;
            result[i] = graph.isConstant(c) ? (graph.node(c).value != 0.0f ? a[i] : b[i]) : floating(graph.select(c, a[i].expr, b[i].expr));
        }
        values[o[1]] = std::move(result);
        return;
    }
    case spv::OpExtInst:
        if (o[2] != glslSet)
            return graph.fail("The shader uses an unsupported extended instruction set.");
        values[o[1]] = glsl(o[0], static_cast<GLSLstd450>(o[3]), o.subspan(4));
        return;
    case spv::OpDot: {
        const std::vector<Scalar>& a = scalars(o[2]);
        const std::vector<Scalar>& b = scalars(o[3]);
        if (a.size() != b.size())
            return graph.fail("A dot product has vectors of different sizes.");
        ExprId sum = zero();
        for (size_t i = 0; i < a.size(); ++i)
            sum = graph.add(sum, graph.mul(a[i].expr, b[i].expr));
        values[o[1]] = std::vector{floating(sum)};
        return;
    }
    case spv::OpVectorTimesScalar: {
        const ExprId factor = scalar(o[3]).expr;
        values[o[1]] = map1(o[2], [&](const Scalar& x) { return floating(graph.mul(x.expr, factor)); });
        return;
    }
    case spv::OpFNegate:
        values[o[1]] = map1(o[2], [&](const Scalar& x) { return floating(graph.negate(x.expr)); });
        return;
    case spv::OpFAdd:
        floatBinary(o, ExprOp::Add);
        return;
    case spv::OpFSub:
        floatBinary(o, ExprOp::Sub);
        return;
    case spv::OpFMul:
        floatBinary(o, ExprOp::Mul);
        return;
    case spv::OpFDiv:
        floatBinary(o, ExprOp::Div);
        return;
    case spv::OpFMod:
        floatBinary(o, ExprOp::Mod);
        return;
    case spv::OpFRem:
        values[o[1]] = map2(o[2], o[3], [&](const Scalar& a, const Scalar& b) { return floating(truncatedRemainder(a.expr, b.expr)); });
        return;
    case spv::OpFOrdEqual:
    case spv::OpFUnordEqual:
        compare(o, ExprOp::Equal, false);
        return;
    case spv::OpFOrdNotEqual:
    case spv::OpFUnordNotEqual:
        compare(o, ExprOp::NotEqual, false);
        return;
    case spv::OpFOrdLessThan:
    case spv::OpFUnordLessThan:
        compare(o, ExprOp::Less, false);
        return;
    case spv::OpFOrdGreaterThan:
    case spv::OpFUnordGreaterThan:
        compare(o, ExprOp::Less, true);
        return;
    case spv::OpFOrdLessThanEqual:
    case spv::OpFUnordLessThanEqual:
        compare(o, ExprOp::LessEqual, false);
        return;
    case spv::OpFOrdGreaterThanEqual:
    case spv::OpFUnordGreaterThanEqual:
        compare(o, ExprOp::LessEqual, true);
        return;
    case spv::OpIEqual:
    case spv::OpLogicalEqual:
        compare(o, ExprOp::Equal, false);
        return;
    case spv::OpINotEqual:
    case spv::OpLogicalNotEqual:
        compare(o, ExprOp::NotEqual, false);
        return;
    case spv::OpSLessThan:
    case spv::OpULessThan:
        compare(o, ExprOp::Less, false);
        return;
    case spv::OpSGreaterThan:
    case spv::OpUGreaterThan:
        compare(o, ExprOp::Less, true);
        return;
    case spv::OpSLessThanEqual:
    case spv::OpULessThanEqual:
        compare(o, ExprOp::LessEqual, false);
        return;
    case spv::OpSGreaterThanEqual:
    case spv::OpUGreaterThanEqual:
        compare(o, ExprOp::LessEqual, true);
        return;
    case spv::OpLogicalAnd:
        values[o[1]] = map2(o[2], o[3], [&](const Scalar& a, const Scalar& b) { return boolean(graph.logicalAnd(a.expr, b.expr)); });
        return;
    case spv::OpLogicalOr:
        values[o[1]] = map2(o[2], o[3], [&](const Scalar& a, const Scalar& b) { return boolean(graph.logicalOr(a.expr, b.expr)); });
        return;
    case spv::OpLogicalNot:
        values[o[1]] = map1(o[2], [&](const Scalar& a) { return boolean(graph.logicalNot(a.expr)); });
        return;
    case spv::OpAny:
    case spv::OpAll: {
        ExprId result = graph.boolean(instruction.op == spv::OpAll);
        for (const Scalar& x : scalars(o[2]))
            result = instruction.op == spv::OpAll ? graph.logicalAnd(result, x.expr) : graph.logicalOr(result, x.expr);
        values[o[1]] = std::vector{boolean(result)};
        return;
    }
    case spv::OpIsNan:
    case spv::OpIsInf:
        values[o[1]] = map1(o[2], [&](const Scalar&) { return boolean(graph.boolean(false)); });
        return;
    case spv::OpBitcast:
        values[o[1]] = map1(o[2], [&](const Scalar& x) { return bitcast(o[0], x); });
        return;
    case spv::OpConvertFToS:
    case spv::OpConvertFToU:
        values[o[1]] = map1(o[2], [&](const Scalar& x) {
            const ExprNode& node = graph.node(x.expr);
            if (node.op == ExprOp::Constant)
                return integer(static_cast<int64_t>(std::trunc(node.value)));
            return Scalar{graph.unary(ExprOp::Trunc, x.expr), std::nullopt};
        });
        return;
    case spv::OpConvertSToF:
    case spv::OpConvertUToF:
    case spv::OpFConvert:
    case spv::OpUConvert:
    case spv::OpSConvert:
    case spv::OpQuantizeToF16:
        values[o[1]] = map1(o[2], [&](const Scalar& x) { return floating(x.expr); });
        return;
    case spv::OpIAdd:
        integerBinary(o, [](int64_t a, int64_t b) { return a + b; }, floatOp(ExprOp::Add));
        return;
    case spv::OpISub:
        integerBinary(o, [](int64_t a, int64_t b) { return a - b; }, floatOp(ExprOp::Sub));
        return;
    case spv::OpIMul:
        integerBinary(o, [](int64_t a, int64_t b) { return a * b; }, floatOp(ExprOp::Mul));
        return;
    case spv::OpUDiv:
        integerBinary(o, [](int64_t a, int64_t b) { return static_cast<uint32_t>(a) / static_cast<uint32_t>(b); }, truncatedQuotient());
        return;
    case spv::OpSDiv:
        integerBinary(o, [](int64_t a, int64_t b) { return static_cast<int32_t>(a) / static_cast<int32_t>(b); }, truncatedQuotient());
        return;
    case spv::OpUMod:
        integerBinary(o, [](int64_t a, int64_t b) { return static_cast<uint32_t>(a) % static_cast<uint32_t>(b); }, floatOp(ExprOp::Mod));
        return;
    case spv::OpSRem:
        values[o[1]] = map2(o[2], o[3], [&](const Scalar& a, const Scalar& b) {
            if (!a.bits || !b.bits)
                return Scalar{truncatedRemainder(a.expr, b.expr), std::nullopt};
            if (*b.bits == 0) {
                graph.fail("The shader takes an integer remainder by zero.");
                return Scalar{zero(), 0u};
            }
            return integer(static_cast<int64_t>(static_cast<int32_t>(*a.bits)) % static_cast<int32_t>(*b.bits));
        });
        return;
    case spv::OpShiftLeftLogical:
        shift(o, [](int64_t a, int64_t b) { return static_cast<uint32_t>(a) << (b & 31); }, true);
        return;
    case spv::OpShiftRightLogical:
        shift(o, [](int64_t a, int64_t b) { return static_cast<uint32_t>(a) >> (b & 31); }, false);
        return;
    case spv::OpShiftRightArithmetic:
        shift(o, [](int64_t a, int64_t b) { return static_cast<int32_t>(a) >> (b & 31); }, false);
        return;
    case spv::OpBitwiseAnd:
        integerBinary(o, [](int64_t a, int64_t b) { return a & b; }, bitwise(Bitwise::And));
        return;
    case spv::OpBitwiseOr:
        integerBinary(o, [](int64_t a, int64_t b) { return a | b; }, bitwise(Bitwise::Or));
        return;
    case spv::OpBitwiseXor:
        integerBinary(o, [](int64_t a, int64_t b) { return a ^ b; }, bitwise(Bitwise::Xor));
        return;
    case spv::OpBitFieldInsert:
    case spv::OpBitFieldSExtract:
    case spv::OpBitFieldUExtract: {
        const bool insert = instruction.op == spv::OpBitFieldInsert;
        const std::vector<Scalar>& base = scalars(o[2]);
        const Scalar offset = scalar(o[insert ? 4 : 3]), count = scalar(o[insert ? 5 : 4]);
        const std::vector<Scalar>& inserted = insert ? scalars(o[3]) : base;
        if (inserted.size() != base.size())
            return graph.fail("A bit field insert has operands of different sizes.");
        std::vector<Scalar> result;
        for (size_t i = 0; i < base.size(); ++i) {
            const std::optional<uint32_t> field = insert ? inserted[i].bits : std::optional<uint32_t>(0u);
            if (!base[i].bits || !field || !offset.bits || !count.bits) {
                result.push_back({graph.unresolved("bits of " + unresolvedNames(base[i].expr)), std::nullopt});
                continue;
            }
            const uint32_t width = *count.bits, shift = *offset.bits;
            const uint32_t mask = width >= 32 ? ~0u : ((1u << width) - 1u);
            uint32_t bits;
            if (insert) {
                bits = (*base[i].bits & ~(mask << shift)) | ((*field & mask) << shift);
            } else {
                bits = (*base[i].bits >> shift) & mask;
                if (instruction.op == spv::OpBitFieldSExtract && width > 0 && width < 32 && (bits >> (width - 1)) & 1u)
                    bits |= ~mask;
            }
            const bool isSigned = types.at(elementType(o[0])).isSigned;
            result.push_back({graph.constant(isSigned ? static_cast<float>(static_cast<int32_t>(bits)) : static_cast<float>(bits)), bits});
        }
        values[o[1]] = std::move(result);
        return;
    }
    case spv::OpSNegate:
        values[o[1]] = map1(o[2], [&](const Scalar& x) {
            return x.bits ? integer(-static_cast<int64_t>(static_cast<int32_t>(*x.bits))) : Scalar{graph.negate(x.expr), std::nullopt};
        });
        return;
    case spv::OpDPdx:
    case spv::OpDPdxFine:
    case spv::OpDPdxCoarse:
        values[o[1]] = map1(o[2], [&](const Scalar& x) { return floating(graph.screenDerivative(x.expr, ScreenAxis::X)); });
        return;
    case spv::OpDPdy:
    case spv::OpDPdyFine:
    case spv::OpDPdyCoarse:
        values[o[1]] = map1(o[2], [&](const Scalar& x) { return floating(graph.screenDerivative(x.expr, ScreenAxis::Y)); });
        return;
    case spv::OpFwidth:
    case spv::OpFwidthFine:
    case spv::OpFwidthCoarse:
        values[o[1]] = map1(o[2], [&](const Scalar& x) {
            return floating(graph.add(graph.unary(ExprOp::Abs, graph.screenDerivative(x.expr, ScreenAxis::X)),
                                      graph.unary(ExprOp::Abs, graph.screenDerivative(x.expr, ScreenAxis::Y))));
        });
        return;
    case spv::OpImageQuerySizeLod: {
        const std::optional<Handle> handle = get<Handle>(o[2]);
        if (!handle)
            return graph.fail("A texture size query has no texture.");
        const auto size = inputs.textureSize(handle->binding, scalar(o[3]).expr);
        std::vector<Scalar> components;
        components.reserve(size.size());
        for (const ExprId component : size)
            components.push_back(graph.isConstant(component) ? integer(static_cast<int64_t>(graph.node(component).value)) : Scalar{component, std::nullopt});
        components.resize(componentCount(o[0]));
        values[o[1]] = std::move(components);
        return;
    }
    default:
        graph.fail("The shader uses the unsupported SPIR-V instruction " + std::to_string(instruction.op) + ".");
    }
}

void SpirvInterpreter::gather(const std::span<const uint32_t> o) {
    const std::optional<Handle> handle = get<Handle>(o[2]);
    const std::vector<Scalar>& coordinate = scalars(o[3]);
    const Scalar component = scalar(o[4]);
    if (graph.isFailed())
        return;
    if (!handle || coordinate.size() < 2)
        return graph.fail("A texture gather has no texture or coordinate.");
    if (!component.bits || *component.bits > 3)
        return graph.fail("A texture gather reads a channel that is not constant.");
    std::array<int64_t, 2> offset{};
    if (o.size() > 5) {
        const uint32_t mask = o[5];
        if ((mask & ~(spv::ImageOperandsConstOffsetMask | spv::ImageOperandsOffsetMask)) != 0 || o.size() != 7)
            return graph.fail("A texture gather has image operands other than an offset.");
        const std::vector<Scalar>& texels = scalars(o[6]);
        if (texels.size() != 2 || !texels[0].bits || !texels[1].bits)
            return graph.fail("A texture gather has an offset that is not constant.");
        offset = {static_cast<int32_t>(*texels[0].bits), static_cast<int32_t>(*texels[1].bits)};
    }
    const std::array<ExprId, 3> size = inputs.textureSize(handle->binding, zero());
    std::array<ExprId, 2> first{};
    for (size_t axis = 0; axis < 2; ++axis)
        first[axis] = graph.add(graph.unary(ExprOp::Floor, graph.sub(graph.mul(coordinate[axis].expr, size[axis]), graph.constant(0.5f))),
                                graph.constant(static_cast<float>(offset[axis]) + 0.5f));
    constexpr std::array<std::array<int, 2>, 4> corners{{{0, 1}, {1, 1}, {1, 0}, {0, 0}}};
    std::vector<Scalar> result;
    for (const auto [x, y] : corners) {
        const std::array<ExprId, 2> centre{graph.div(graph.add(first[0], graph.constant(static_cast<float>(x))), size[0]),
                                           graph.div(graph.add(first[1], graph.constant(static_cast<float>(y))), size[1])};
        result.push_back(floating(inputs.sample(handle->binding, centre)[*component.bits]));
    }
    values[o[1]] = std::move(result);
}

Scalar SpirvInterpreter::boolean(const ExprId expr) const {
    const ExprNode& node = graph.node(expr);
    if (node.op == ExprOp::Constant)
        return {expr, node.value != 0.0f ? 1u : 0u};
    return {expr, std::nullopt};
}

Scalar SpirvInterpreter::bitcast(const uint32_t typeId, const Scalar& x) {
    uint32_t element = typeId;
    if (types.at(element).op == spv::OpTypeVector)
        element = types.at(element).element;
    if (!x.bits)
        return {x.expr, std::nullopt};
    return constantScalar(element, *x.bits);
}

void SpirvInterpreter::floatBinary(const std::span<const uint32_t> o, const ExprOp op) {
    values[o[1]] = map2(o[2], o[3], [&](const Scalar& a, const Scalar& b) { return floating(graph.binary(op, a.expr, b.expr)); });
}

void SpirvInterpreter::compare(const std::span<const uint32_t> o, const ExprOp op, const bool swapped) {
    values[o[1]] = map2(
        o[2], o[3], [&](const Scalar& a, const Scalar& b) { return boolean(swapped ? graph.binary(op, b.expr, a.expr) : graph.binary(op, a.expr, b.expr)); });
}

uint32_t SpirvInterpreter::elementType(const uint32_t typeId) const {
    const Type& type = types.at(typeId);
    return type.op == spv::OpTypeVector ? type.element : typeId;
}

}
