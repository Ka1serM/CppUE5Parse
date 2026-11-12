#include "Conversion/Spirv/SpirvInterpreter.h"

#include "Conversion/Shaders/StageBindings.h"

namespace ue5parse::conversion {

void SpirvInterpreter::parse(const std::span<const uint32_t> words) {
    if (words.size() < 5 || words[0] != spv::MagicNumber)
        return graph.fail("The shader is not a SPIR-V module.");
    uint32_t currentFunction = 0;
    uint32_t entry = 0;
    for (size_t at = 5; at < words.size() && !graph.isFailed();) {
        const uint32_t count = words[at] >> 16;
        const auto op = static_cast<spv::Op>(words[at] & 0xffff);
        if (count == 0 || at + count > words.size())
            return graph.fail("The SPIR-V module has a malformed instruction.");
        const std::span<const uint32_t> operands = words.subspan(at + 1, count - 1);
        at += count;
        switch (op) {
        case spv::OpEntryPoint:
            if (operands[0] == spv::ExecutionModelFragment || operands[0] == spv::ExecutionModelVertex)
                entry = operands[1];
            break;
        case spv::OpExtInstImport:
            if (literalString(operands.subspan(1)) == "GLSL.std.450")
                glslSet = operands[0];
            break;
        case spv::OpDecorate:
            decorate(operands[0], static_cast<spv::Decoration>(operands[1]), operands.subspan(2));
            break;
        case spv::OpMemberDecorate:
            if (static_cast<spv::Decoration>(operands[2]) == spv::DecorationOffset)
                memberOffsets[operands[0]][operands[1]] = operands[3];
            break;
        case spv::OpTypeVoid:
        case spv::OpTypeBool:
        case spv::OpTypeSampler:
            types[operands[0]] = {.op = op};
            break;
        case spv::OpTypeInt:
            types[operands[0]] = {.op = op, .isSigned = operands[2] != 0};
            break;
        case spv::OpTypeFloat:
            types[operands[0]] = {.op = op};
            break;
        case spv::OpTypeVector:
        case spv::OpTypeMatrix:
            types[operands[0]] = {.op = op, .element = operands[1], .count = operands[2]};
            break;
        case spv::OpTypeArray: {
            const std::optional<uint32_t> length = scalar(operands[2]).bits;
            if (!length)
                return graph.fail("The SPIR-V module has an array without a constant length.");
            types[operands[0]] = {.op = op, .element = operands[1], .count = *length};
            break;
        }
        case spv::OpTypeRuntimeArray:
            types[operands[0]] = {.op = op, .element = operands[1], .count = 0};
            break;
        case spv::OpTypeStruct:
            types[operands[0]] = {.op = op, .members = {operands.begin() + 1, operands.end()}};
            break;
        case spv::OpTypePointer:
            types[operands[0]] = {.op = op, .element = operands[2], .storage = static_cast<spv::StorageClass>(operands[1])};
            break;
        case spv::OpTypeImage:
        case spv::OpTypeSampledImage:
            types[operands[0]] = {.op = op, .element = operands[1]};
            break;
        case spv::OpTypeFunction:
            break;
        case spv::OpConstant:
        case spv::OpSpecConstant:
            valueTypes[operands[1]] = operands[0];
            values[operands[1]] = std::vector{constantScalar(operands[0], operands[2])};
            break;
        case spv::OpConstantTrue:
        case spv::OpConstantFalse:
        case spv::OpSpecConstantTrue:
        case spv::OpSpecConstantFalse: {
            const bool value = op == spv::OpConstantTrue || op == spv::OpSpecConstantTrue;
            values[operands[1]] = std::vector{Scalar{graph.boolean(value), value ? 1u : 0u}};
            break;
        }
        case spv::OpConstantComposite:
        case spv::OpSpecConstantComposite: {
            valueTypes[operands[1]] = operands[0];
            std::vector<Scalar> result;
            for (const uint32_t part : operands.subspan(2))
                append(result, scalars(part));
            values[operands[1]] = std::move(result);
            break;
        }
        case spv::OpConstantNull:
        case spv::OpUndef:
            valueTypes[operands[1]] = operands[0];
            values[operands[1]] = std::vector<Scalar>(componentCount(operands[0]), Scalar{zero(), 0u});
            break;
        case spv::OpVariable:
            if (currentFunction == 0) {
                variables[operands[1]] = {types.at(operands[0]).element, static_cast<spv::StorageClass>(operands[2]),
                                          operands.size() > 3 ? std::optional(operands[3]) : std::nullopt};
                values[operands[1]] = Pointer{operands[1], types.at(operands[0]).element, 0, 0};
            } else
                blocks[currentBlock].push_back({op, operands});
            break;
        case spv::OpFunction:
            if (operands[1] != entry)
                return graph.fail("The SPIR-V module has a function besides its entry point.");
            currentFunction = operands[1];
            break;
        case spv::OpFunctionEnd:
            currentFunction = 0;
            break;
        case spv::OpLabel:
            currentBlock = operands[0];
            if (entryBlock == 0)
                entryBlock = currentBlock;
            blocks[currentBlock] = {};
            break;
        default:
            if (currentFunction != 0)
                blocks[currentBlock].push_back({op, operands});
            break;
        }
    }
    if (entryBlock == 0)
        graph.fail("The SPIR-V module has no fragment entry point.");
}

void SpirvInterpreter::decorate(const uint32_t id, const spv::Decoration decoration, const std::span<const uint32_t> values) {
    switch (decoration) {
    case spv::DecorationLocation:
        locations[id] = values[0];
        break;
    case spv::DecorationBinding:
        bindings[id] = values[0] % stageBindings;
        break;
    case spv::DecorationBuiltIn:
        builtins[id] = static_cast<spv::BuiltIn>(values[0]);
        break;
    case spv::DecorationArrayStride:
        arrayStrides[id] = values[0];
        break;
    default:
        break;
    }
}

std::string SpirvInterpreter::literalString(const std::span<const uint32_t> words) {
    std::string result;
    for (const uint32_t word : words)
        for (int byte = 0; byte < 4; ++byte) {
            const char character = static_cast<char>((word >> (byte * 8)) & 0xff);
            if (character == '\0')
                return result;
            result += character;
        }
    return result;
}

Scalar SpirvInterpreter::constantScalar(const uint32_t type, const uint32_t bits) {
    if (types.at(type).op == spv::OpTypeFloat)
        return {graph.constant(std::bit_cast<float>(bits)), bits};
    const float value = types.at(type).isSigned ? static_cast<float>(static_cast<int32_t>(bits)) : static_cast<float>(bits);
    return {graph.constant(value), bits};
}

Scalar SpirvInterpreter::floating(const ExprId expr) const {
    const ExprNode& node = graph.node(expr);
    if (node.op == ExprOp::Constant && !graph.isBoolean(expr))
        return {expr, std::bit_cast<uint32_t>(node.value)};
    return {expr, std::nullopt};
}

uint32_t SpirvInterpreter::componentCount(const uint32_t typeId) const {
    const Type& type = types.at(typeId);
    switch (type.op) {
    case spv::OpTypeVector:
    case spv::OpTypeMatrix:
    case spv::OpTypeArray:
        return type.count * componentCount(type.element);
    case spv::OpTypeStruct: {
        uint32_t total = 0;
        for (const uint32_t member : type.members)
            total += componentCount(member);
        return total;
    }
    default:
        return 1;
    }
}

const std::vector<Scalar>& SpirvInterpreter::scalars(const uint32_t id) {
    static const std::vector<Scalar> none;
    const auto found = values.find(id);
    if (found == values.end()) {
        graph.fail("The SPIR-V value %" + std::to_string(id) + " is not available.");
        return none;
    }
    if (const auto* result = std::get_if<std::vector<Scalar>>(&found->second))
        return *result;
    graph.fail("The SPIR-V value %" + std::to_string(id) + " is not a plain value.");
    return none;
}

Scalar SpirvInterpreter::scalar(const uint32_t id) {
    const std::vector<Scalar>& result = scalars(id);
    if (result.empty()) {
        graph.fail("The SPIR-V value %" + std::to_string(id) + " has no components.");
        return {zero(), 0u};
    }
    return result[0];
}

std::vector<Scalar> SpirvInterpreter::initialValue(const Variable& variable) {
    if (variable.initializer)
        return scalars(*variable.initializer);
    return std::vector<Scalar>(componentCount(variable.type), Scalar{zero(), 0u});
}

}
