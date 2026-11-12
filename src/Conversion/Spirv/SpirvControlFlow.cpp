#include "Conversion/Spirv/SpirvInterpreter.h"

namespace ue5parse::conversion {

std::optional<Incoming> SpirvInterpreter::runRegion(uint32_t block, const uint32_t stop, Incoming incoming, ExprId path) {
    while (!graph.isFailed()) {
        if (block == stop)
            return incoming;
        for (const Loop& loop : loops)
            if (loop.forkDepth < forkDepth && (block == loop.header || block == loop.merge || block == loop.continueTarget))
                return failRegion("A loop exit depends on values the translation cannot express: " + unresolvedNames(forkConditions.back()));
        if (!loops.empty() && block == loops.back().merge) {
            if (!loops.back().exits.empty()) {
                path = joinExits(loops.back(), incoming.predecessor);
                incoming = {.phisResolved = true};
            }
            loops.pop_back();
        }
        const std::vector<Instruction>& instructions = blocks.at(block);
        std::optional<uint32_t> merge;
        for (const Instruction& instruction : instructions) {
            const auto& operands = instruction.operands;
            switch (instruction.op) {
            case spv::OpPhi:
                valueTypes[operands[1]] = operands[0];
                if (incoming.phisResolved)
                    break;
                values[operands[1]] = phi(operands, incoming);
                if (graph.isFailed())
                    return std::nullopt;
                break;
            case spv::OpSelectionMerge:
                merge = operands[0];
                break;
            case spv::OpLoopMerge:
                if (loops.empty() || loops.back().header != block)
                    loops.push_back({.header = block, .merge = operands[0], .continueTarget = operands[1], .forkDepth = forkDepth, .entryPath = path});
                if (++loops.back().iterations > maxLoopIterations)
                    return failRegion("A loop runs more than " + std::to_string(maxLoopIterations) + " iterations.");
                if (loops.back().unknownIterations >= maxUnknownLoopIterations) {
                    notes.insert("a loop that exits on runtime values runs at most " + std::to_string(maxUnknownLoopIterations) + " iterations");
                    path = joinExits(loops.back(), std::nullopt);
                    incoming = {.phisResolved = true};
                    block = loops.back().merge;
                    loops.pop_back();
                    goto next;
                }
                break;
            case spv::OpBranch:
                incoming = {.predecessor = block};
                block = operands[0];
                goto next;
            case spv::OpBranchConditional: {
                const ExprId condition = scalar(operands[0]).expr;
                if (graph.isConstant(condition)) {
                    incoming = {.predecessor = block};
                    block = graph.node(condition).value != 0.0f ? operands[1] : operands[2];
                    goto next;
                }
                const bool inLoop = !loops.empty() && loops.back().forkDepth == forkDepth;
                const bool exitsOnTrue = inLoop && leavesLoop(operands[1], loops.back()) && !leavesLoop(operands[2], loops.back());
                const bool exitsOnFalse = inLoop && leavesLoop(operands[2], loops.back()) && !leavesLoop(operands[1], loops.back());
                if (!merge && !exitsOnTrue && !exitsOnFalse)
                    return failRegion("The shader has an unstructured conditional branch.");
                if (exitsOnTrue || exitsOnFalse) {
                    const ExprId exitCondition = exitsOnTrue ? condition : graph.logicalNot(condition);
                    recordExit(loops.back(), block, exitsOnTrue ? operands[1] : operands[2], exitCondition, path);
                    path = graph.logicalAnd(path, graph.logicalNot(exitCondition));
                    incoming = {.predecessor = block};
                    block = exitsOnTrue ? operands[2] : operands[1];
                    goto next;
                }
                auto joined = fork(block, operands[1], operands[2], *merge, condition, path);
                if (!joined)
                    return std::nullopt;
                incoming = *joined;
                block = *merge;
                goto next;
            }
            case spv::OpSwitch: {
                const Scalar selector = scalar(operands[0]);
                if (!selector.bits)
                    return failRegion("The shader switches on a value that is not constant.");
                uint32_t target = operands[1];
                for (size_t i = 2; i + 1 < operands.size(); i += 2)
                    if (operands[i] == *selector.bits)
                        target = operands[i + 1];
                incoming = {.predecessor = block};
                block = target;
                goto next;
            }
            case spv::OpReturn:
                return std::nullopt;
            case spv::OpKill:
            case spv::OpTerminateInvocation:
                discard = graph.logicalOr(discard, path);
                return std::nullopt;
            case spv::OpUnreachable:
                return std::nullopt;
            default:
                execute(instruction, path);
                if (graph.isFailed())
                    return std::nullopt;
                break;
            }
        }
        return failRegion("A SPIR-V block has no terminator.");
    next:;
    }
    return std::nullopt;
}

std::optional<Incoming> SpirvInterpreter::failRegion(std::string message) {
    graph.fail(std::move(message));
    return std::nullopt;
}

std::optional<Incoming> SpirvInterpreter::fork(const uint32_t header, const uint32_t trueBlock, const uint32_t falseBlock, const uint32_t merge,
                                               const ExprId condition, const ExprId path) {
    Memory before = memory;
    ++forkDepth;
    forkConditions.push_back(condition);
    const auto trueArm = runRegion(trueBlock, merge, {.predecessor = header}, graph.logicalAnd(path, condition));
    Memory trueMemory = std::exchange(memory, std::move(before));
    const auto falseArm = runRegion(falseBlock, merge, {.predecessor = header}, graph.logicalAnd(path, graph.logicalNot(condition)));
    --forkDepth;
    forkConditions.pop_back();
    if (!trueArm && !falseArm)
        return std::nullopt;
    if (!falseArm) {
        memory = std::move(trueMemory);
        return trueArm;
    }
    if (!trueArm)
        return falseArm;
    mergeMemory(condition, trueMemory, memory);
    if (trueArm->fork || falseArm->fork)
        return failRegion("Two selections share a merge block.");
    return Incoming{.condition = condition, .truePredecessor = trueArm->predecessor, .falsePredecessor = falseArm->predecessor, .fork = true};
}

bool SpirvInterpreter::leavesLoop(const uint32_t target, const Loop& loop) const {
    if (target == loop.merge)
        return true;
    const std::vector<Instruction>& instructions = blocks.at(target);
    if (instructions.empty() || instructions.back().op != spv::OpBranch || instructions.back().operands[0] != loop.merge)
        return false;
    return std::ranges::none_of(instructions, [](const Instruction& instruction) {
        return instruction.op == spv::OpPhi || instruction.op == spv::OpSelectionMerge || instruction.op == spv::OpLoopMerge;
    });
}

void SpirvInterpreter::recordExit(Loop& loop, const uint32_t block, const uint32_t target, const ExprId condition, const ExprId path) {
    if (loop.exits.empty() || loop.exits.back().iteration != loop.iterations)
        ++loop.unknownIterations;
    if (target == loop.merge) {
        loop.exits.push_back({condition, memory, values, block, loop.iterations});
        return;
    }
    Memory memoryBefore = memory;
    std::unordered_map<uint32_t, Value> valuesBefore = values;
    const std::vector<Instruction>& instructions = blocks.at(target);
    for (size_t i = 0; i + 1 < instructions.size() && !graph.isFailed(); ++i)
        execute(instructions[i], graph.logicalAnd(path, condition));
    loop.exits.push_back({condition, std::move(memory), std::move(values), target, loop.iterations});
    memory = std::move(memoryBefore);
    values = std::move(valuesBefore);
}

ExprId SpirvInterpreter::joinExits(Loop& loop, const std::optional<uint32_t> predecessor) {
    std::vector<LoopExit>& exits = loop.exits;
    if (predecessor) {
        resolvePhis(loop.merge, *predecessor, values);
    } else {
        resolvePhis(loop.merge, exits.back().predecessor, exits.back().values);
        memory = std::move(exits.back().memory);
        values = std::move(exits.back().values);
        exits.pop_back();
    }
    for (auto exit = exits.rbegin(); exit != exits.rend(); ++exit) {
        resolvePhis(loop.merge, exit->predecessor, exit->values);
        mergeMemory(exit->condition, exit->memory, memory);
        for (auto& [id, value] : values) {
            const auto taken = exit->values.find(id);
            const auto* current = std::get_if<std::vector<Scalar>>(&value);
            const auto* earlier = taken == exit->values.end() ? nullptr : std::get_if<std::vector<Scalar>>(&taken->second);
            if (current && earlier && current->size() == earlier->size() && *current != *earlier)
                value = selectValues(exit->condition, *earlier, *current);
        }
    }
    return loop.entryPath;
}

void SpirvInterpreter::resolvePhis(const uint32_t block, const uint32_t predecessor, std::unordered_map<uint32_t, Value>& state) {
    for (const Instruction& instruction : blocks.at(block)) {
        if (instruction.op != spv::OpPhi)
            return;
        const auto& operands = instruction.operands;
        valueTypes[operands[1]] = operands[0];
        std::optional<uint32_t> operand;
        for (size_t i = 2; i + 1 < operands.size(); i += 2)
            if (operands[i + 1] == predecessor)
                operand = operands[i];
        const auto found = operand ? state.find(*operand) : state.end();
        if (found == state.end())
            return graph.fail("A phi after a loop has no value for the block the loop leaves from.");
        Value value = found->second;
        state[operands[1]] = std::move(value);
    }
}

Value SpirvInterpreter::phi(const std::span<const uint32_t> operands, const Incoming& incoming) {
    const auto operand = [&](const uint32_t predecessor) -> uint32_t {
        for (size_t i = 2; i + 1 < operands.size(); i += 2)
            if (operands[i + 1] == predecessor)
                return operands[i];
        graph.fail("A phi has no operand for its predecessor.");
        return 0;
    };
    if (!incoming.fork) {
        const auto found = values.find(operand(incoming.predecessor));
        if (found == values.end()) {
            graph.fail("A phi reads a value that is not available.");
            return std::vector<Scalar>();
        }
        return found->second;
    }
    return selectValues(incoming.condition, scalars(operand(incoming.truePredecessor)), scalars(operand(incoming.falsePredecessor)));
}

std::vector<Scalar> SpirvInterpreter::selectValues(const ExprId condition, const std::vector<Scalar>& a, const std::vector<Scalar>& b) {
    if (a.size() != b.size()) {
        graph.fail("A selection joins values of different sizes.");
        return {};
    }
    std::vector<Scalar> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].expr == b[i].expr && a[i].bits == b[i].bits) {
            result[i] = a[i];
            continue;
        }
        result[i] = floating(graph.select(condition, a[i].expr, b[i].expr));
    }
    return result;
}

void SpirvInterpreter::mergeMemory(const ExprId condition, const Memory& taken, Memory& other) {
    for (const auto& [id, value] : taken) {
        const auto [found, added] = other.try_emplace(id, value);
        if (!added && found->second != value)
            found->second = selectValues(condition, value, found->second);
    }
}

}
