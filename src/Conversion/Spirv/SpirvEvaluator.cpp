#include "Conversion/Spirv/SpirvInterpreter.h"

namespace ue5parse::conversion {

ShaderOutputs SpirvInterpreter::run() {
    discard = graph.boolean(false);
    if (graph.isFailed())
        return {};
    for (const auto& [id, variable] : variables)
        if (variable.storage == spv::StorageClassOutput || variable.storage == spv::StorageClassPrivate)
            memory[id] = initialValue(variable);
    runRegion(entryBlock, 0, {}, graph.boolean(true));

    ShaderOutputs outputs;
    outputs.discard = discard;
    outputs.notes.assign(notes.begin(), notes.end());
    for (const auto& [id, variable] : variables) {
        if (variable.storage != spv::StorageClassOutput)
            continue;
        if (const auto builtin = builtins.find(id); builtin != builtins.end()) {
            if (builtin->second == spv::BuiltInPosition)
                for (size_t i = 0; i < 4; ++i)
                    outputs.position[i] = memory.at(id)[i].expr;
            continue;
        }
        const Type& type = types.at(variable.type);
        const uint32_t row = type.op == spv::OpTypeArray ? componentCount(type.element) : componentCount(variable.type);
        const std::vector<Scalar>& value = memory.at(id);
        for (size_t i = 0; i < value.size(); ++i) {
            const uint32_t location = locations.at(id) + static_cast<uint32_t>(i) / row;
            if (outputs.targets.size() <= location)
                outputs.targets.resize(location + 1, {zero(), zero(), zero(), zero()});
            if (i % row < 4)
                outputs.targets[location][i % row] = value[i].expr;
        }
    }
    return outputs;
}

std::string SpirvInterpreter::unresolvedNames(const ExprId value) const {
    std::string names;
    for (const ExprId id : graph.unresolvedInputs(value))
        names += (names.empty() ? "" : ", ") + graph.unresolvedName(id);
    return names.empty() ? "(geometry or textures)" : names;
}

ShaderOutputs evaluateSpirv(const std::span<const uint32_t> words, ExpressionGraph& graph, ShaderInputs& inputs) {
    return SpirvInterpreter(words, graph, inputs).run();
}

}
