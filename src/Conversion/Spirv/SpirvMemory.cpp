#include "Conversion/Spirv/SpirvInterpreter.h"

namespace ue5parse::conversion {

std::pair<uint32_t, uint32_t> SpirvInterpreter::compositeRange(const uint32_t composite, const std::span<const uint32_t> indices) const {
    uint32_t typeId = valueTypes.at(composite);
    uint32_t first = 0;
    for (const uint32_t index : indices) {
        const Type& type = types.at(typeId);
        if (type.op == spv::OpTypeStruct) {
            for (uint32_t i = 0; i < index; ++i)
                first += componentCount(type.members[i]);
            typeId = type.members.at(index);
        } else {
            first += index * componentCount(type.element);
            typeId = type.element;
        }
    }
    return {first, componentCount(typeId)};
}

Pointer SpirvInterpreter::accessChain(const uint32_t base, const std::span<const uint32_t> indices) {
    Pointer pointer = get<Pointer>(base).value_or(Pointer());
    for (const uint32_t indexId : indices) {
        if (graph.isFailed())
            break;
        const Scalar index = scalar(indexId);
        if (!index.bits) {
            graph.fail("The shader indexes with a value that is not constant.");
            break;
        }
        step(pointer, *index.bits);
    }
    return pointer;
}

void SpirvInterpreter::step(Pointer& pointer, const uint32_t index) {
    const Type& type = types.at(pointer.type);
    switch (type.op) {
    case spv::OpTypeStruct: {
        for (uint32_t i = 0; i < index; ++i)
            pointer.component += componentCount(type.members[i]);
        if (const auto offsets = memberOffsets.find(pointer.type); offsets != memberOffsets.end())
            if (const auto offset = offsets->second.find(index); offset != offsets->second.end())
                pointer.offset += offset->second;
        if (index >= type.members.size())
            return graph.fail("An access chain selects a member that its struct does not have.");
        pointer.type = type.members[index];
        return;
    }
    case spv::OpTypeArray:
    case spv::OpTypeRuntimeArray:
    case spv::OpTypeMatrix: {
        pointer.component += index * componentCount(type.element);
        const auto stride = arrayStrides.find(pointer.type);
        pointer.offset += index * (stride != arrayStrides.end() ? stride->second : 16u);
        pointer.type = type.element;
        return;
    }
    case spv::OpTypeVector:
        pointer.component += index;
        pointer.offset += index * 4;
        pointer.type = type.element;
        return;
    default:
        graph.fail("An access chain goes into a scalar.");
    }
}

void SpirvInterpreter::uniformOffsets(const uint32_t typeId, const uint32_t offset, std::vector<std::pair<uint32_t, uint32_t>>& result) const {
    const Type& type = types.at(typeId);
    switch (type.op) {
    case spv::OpTypeStruct: {
        const auto& offsets = memberOffsets.at(typeId);
        for (uint32_t i = 0; i < type.members.size(); ++i)
            uniformOffsets(type.members[i], offset + offsets.at(i), result);
        return;
    }
    case spv::OpTypeArray: {
        const auto stride = arrayStrides.find(typeId);
        for (uint32_t i = 0; i < type.count; ++i)
            uniformOffsets(type.element, offset + i * (stride != arrayStrides.end() ? stride->second : 16u), result);
        return;
    }
    case spv::OpTypeVector:
        for (uint32_t i = 0; i < type.count; ++i)
            result.emplace_back(offset + i * 4, type.element);
        return;
    default:
        result.emplace_back(offset, typeId);
    }
}

Value SpirvInterpreter::load(const uint32_t pointerId) {
    const std::optional<Pointer> found = get<Pointer>(pointerId);
    if (!found)
        return std::vector<Scalar>();
    const Pointer& pointer = *found;
    const Variable& variable = variables.at(pointer.variable);
    switch (variable.storage) {
    case spv::StorageClassUniform: {
        std::vector<std::pair<uint32_t, uint32_t>> offsets;
        uniformOffsets(pointer.type, pointer.offset, offsets);
        std::vector<Scalar> result;
        const uint32_t reg = bindings.at(pointer.variable);
        for (const auto [offset, scalarType] : offsets) {
            Scalar value = floating(inputs.constantBuffer(reg, offset));
            if (types.at(scalarType).op != spv::OpTypeFloat && value.bits)
                value = constantScalar(scalarType, *value.bits);
            result.push_back(value);
        }
        return result;
    }
    case spv::StorageClassUniformConstant:
        return Handle{bindings.at(pointer.variable)};
    case spv::StorageClassInput: {
        std::vector<Scalar> result;
        const uint32_t count = componentCount(pointer.type);
        if (const auto builtin = builtins.find(pointer.variable); builtin != builtins.end()) {
            for (uint32_t i = 0; i < count; ++i) {
                if (builtin->second == spv::BuiltInFrontFacing) {
                    result.push_back(boolean(inputs.frontFacing()));
                } else if (builtin->second == spv::BuiltInFragCoord) {
                    result.push_back(floating(inputs.fragCoord(pointer.component + i)));
                } else if (builtin->second == spv::BuiltInVertexIndex || builtin->second == spv::BuiltInInstanceIndex
                           || builtin->second == spv::BuiltInBaseVertex || builtin->second == spv::BuiltInBaseInstance) {
                    result.push_back(Scalar{graph.constant(0.0f), 0u});
                } else {
                    graph.fail("The shader reads the unsupported built-in input " + std::to_string(builtin->second) + ".");
                    return std::vector<Scalar>();
                }
            }
            return result;
        }
        const Type& type = types.at(variable.type);
        const uint32_t row = type.op == spv::OpTypeArray ? componentCount(type.element) : componentCount(variable.type);
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t scalar = pointer.component + i;
            result.push_back(floating(inputs.input(locations.at(pointer.variable) + scalar / row, scalar % row)));
        }
        return result;
    }
    case spv::StorageClassStorageBuffer:
        return std::vector<Scalar>(componentCount(pointer.type), Scalar{zero(), 0u});
    default: {
        const auto stored = memory.find(pointer.variable);
        const uint32_t count = componentCount(pointer.type);
        if (stored == memory.end() || pointer.component + count > stored->second.size()) {
            graph.fail("The shader loads from a variable that it has not written.");
            return std::vector<Scalar>();
        }
        return std::vector<Scalar>(stored->second.begin() + pointer.component, stored->second.begin() + pointer.component + count);
    }
    }
}

void SpirvInterpreter::store(const uint32_t pointerId, const std::vector<Scalar>& value) {
    const std::optional<Pointer> pointer = get<Pointer>(pointerId);
    if (!pointer)
        return;
    const spv::StorageClass storage = variables.at(pointer->variable).storage;
    if (storage == spv::StorageClassUniform || storage == spv::StorageClassInput)
        return graph.fail("The shader stores to a read-only variable.");
    if (storage == spv::StorageClassStorageBuffer)
        return;
    const auto stored = memory.find(pointer->variable);
    if (stored == memory.end() || pointer->component + value.size() > stored->second.size())
        return graph.fail("The shader stores past its variable.");
    std::copy(value.begin(), value.end(), stored->second.begin() + pointer->component);
}

}
