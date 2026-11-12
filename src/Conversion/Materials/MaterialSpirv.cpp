#define SPV_ENABLE_UTILITY_CODE
#include "Conversion/Materials/MaterialSpirv.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <spirv/unified1/GLSL.std.450.h>
#include <spirv/unified1/spirv.hpp>

namespace ue5parse::conversion {
namespace {

enum class Stage : uint8_t { Vertex, Fragment };

struct Instruction {
    spv::Op op;
    std::span<const uint32_t> operands;
};

std::string_view literalString(const std::span<const uint32_t> words) {
    const auto* text = reinterpret_cast<const char*>(words.data());
    return {text, strnlen(text, words.size() * sizeof(uint32_t))};
}

std::optional<uint32_t> resultId(const Instruction& instruction) {
    bool hasResult = false;
    bool hasType = false;
    spv::HasResultAndType(instruction.op, &hasResult, &hasType);
    const std::size_t at = hasType ? 1 : 0;
    if (!hasResult || instruction.operands.size() <= at)
        return std::nullopt;
    return instruction.operands[at];
}

std::vector<uint32_t> operands(const ProgramInstruction& instruction) {
    switch (instruction.op) {
    case ProgramOp::Constant:
    case ProgramOp::Parameter:
    case ProgramOp::Geometry:
    case ProgramOp::GeometryDx:
    case ProgramOp::GeometryDy:
    case ProgramOp::Time:
    case ProgramOp::Frame:
        return {};
    case ProgramOp::Texture:
    case ProgramOp::Extract:
    case ProgramOp::Abs:
    case ProgramOp::Floor:
    case ProgramOp::Ceil:
    case ProgramOp::Fract:
    case ProgramOp::Round:
    case ProgramOp::Trunc:
    case ProgramOp::Sign:
    case ProgramOp::Sqrt:
    case ProgramOp::Exp2:
    case ProgramOp::Log2:
    case ProgramOp::Sin:
    case ProgramOp::Cos:
    case ProgramOp::Tan:
    case ProgramOp::Asin:
    case ProgramOp::Acos:
    case ProgramOp::Atan:
    case ProgramOp::Random:
    case ProgramOp::Length:
    case ProgramOp::Normalize:
    case ProgramOp::TangentToWorld:
        return {instruction.a};
    case ProgramOp::Combine:
        if (instruction.width >= 3)
            return {instruction.a, instruction.b, instruction.c};
        return {instruction.a, instruction.b};
    case ProgramOp::Select:
    case ProgramOp::Clamp:
    case ProgramOp::Mix:
    case ProgramOp::Smoothstep:
    case ProgramOp::MulAdd:
        return {instruction.a, instruction.b, instruction.c};
    default:
        return {instruction.a, instruction.b};
    }
}

}

class MaterialShell::Module {
public:
    explicit Module(const std::span<const uint32_t> words) : words(words) {}

    std::optional<std::string> read() {
        if (words.size() < 5 || words[0] != spv::MagicNumber)
            return "The material shell is not a SPIR-V module.";
        bound = words[3];
        std::multimap<std::string, uint32_t, std::less<>> names;
        for (std::size_t at = 5; at < words.size();) {
            const uint32_t count = words[at] >> 16;
            if (count == 0 || at + count > words.size())
                return "The material shell has a malformed instruction.";
            const Instruction instruction{static_cast<spv::Op>(words[at] & 0xffff), words.subspan(at + 1, count - 1)};
            instructions.push_back(instruction);
            at += count;
            const std::span<const uint32_t> operand = instruction.operands;
            switch (instruction.op) {
            case spv::OpName:
                names.emplace(std::string(literalString(operand.subspan(1))), operand[0]);
                break;
            case spv::OpExtInstImport:
                if (literalString(operand.subspan(1)) == "GLSL.std.450")
                    glsl = operand[0];
                break;
            case spv::OpTypeVoid:
                voidType = operand[0];
                break;
            case spv::OpTypeBool:
                boolType = operand[0];
                break;
            case spv::OpTypeFloat:
                if (operand[1] == 32 && operand.size() == 2)
                    floatType = operand[0];
                break;
            case spv::OpTypeInt:
                if (operand[1] == 32 && operand[2] == 0)
                    uintType = operand[0];
                break;
            case spv::OpTypeVector:
                vectorTypes[{operand[1], operand[2]}] = operand[0];
                break;
            case spv::OpTypeFunction:
                functionTypes[operand[0]] = {operand.begin() + 1, operand.end()};
                break;
            case spv::OpFunction:
                functions[operand[1]] = operand[3];
                break;
            default:
                break;
            }
        }
        for (const auto& [name, id] : names)
            if (functions.contains(id))
                functionNames.emplace(name, id);
        return std::nullopt;
    }

    std::optional<uint32_t> function(const std::string_view name) const {
        const auto found = functionNames.find(name);
        if (found == functionNames.end())
            return std::nullopt;
        return found->second;
    }

    const std::vector<uint32_t>& signature(const uint32_t function) const { return functionTypes.at(functions.at(function)); }

    std::span<const uint32_t> words;
    std::vector<Instruction> instructions;
    uint32_t bound = 0;
    uint32_t glsl = 0;
    uint32_t voidType = 0;
    uint32_t boolType = 0;
    uint32_t floatType = 0;
    uint32_t uintType = 0;
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> vectorTypes;
    std::unordered_map<uint32_t, std::vector<uint32_t>> functionTypes;
    std::unordered_map<uint32_t, uint32_t> functions;
    std::map<std::string, uint32_t, std::less<>> functionNames;
};

namespace {

class Writer {
public:
    Writer(const MaterialShell::Module& shell, const MaterialProgram& program)
        : shell(shell), program(program), bound(shell.bound), vectorTypes(shell.vectorTypes) {}

    ConversionResult<MaterialSpirv> write() {
        const std::optional<uint32_t> material = shell.function("evaluateMaterial");
        if (!material)
            return conversionFailure("The material shell has no evaluateMaterial function.");
        prepareTypes();
        bodies[*material] = writeSurface();
        materialFunctions.push_back(*material);
        if (const std::optional<uint32_t> displacement = shell.function("evaluateDisplacement")) {
            bodies[*displacement] = writeDisplacement();
            if (program.displacement >= 0)
                materialFunctions.push_back(*displacement);
        }
        if (error)
            return conversionFailure(std::move(*error));
        for (const auto& [function, body] : bodies) {
            const std::vector<uint32_t>& type = shell.signature(function);
            if (type.size() != 1 || type[0] != shell.voidType)
                return conversionFailure("A material shell entry must take no parameters and return void.");
        }
        const std::unordered_set<uint32_t> kept = keptFunctions();
        if (!kept.contains(*material))
            return conversionFailure("No entry point of the material shell calls evaluateMaterial.");
        return MaterialSpirv{.words = assemble(kept), .constants = std::move(constants)};
    }

private:
    void prepareTypes() {
        if (!shell.glsl)
            glsl = next();
        else
            glsl = shell.glsl;
        voidType = shell.voidType ? shell.voidType : global(spv::OpTypeVoid, {});
        boolType = shell.boolType ? shell.boolType : global(spv::OpTypeBool, {});
        floatType = shell.floatType ? shell.floatType : global(spv::OpTypeFloat, {32});
        uintType = shell.uintType ? shell.uintType : global(spv::OpTypeInt, {32, 0});
        float3Type = vectorType(floatType, 3);
        float4Type = vectorType(floatType, 4);
        bool4Type = vectorType(boolType, 4);
        zero = splatConstant(0.0f);
        one = splatConstant(1.0f);
    }

    uint32_t next() { return bound++; }

    uint32_t vectorType(const uint32_t component, const uint32_t count) {
        const auto found = vectorTypes.find({component, count});
        if (found != vectorTypes.end())
            return found->second;
        const uint32_t id = global(spv::OpTypeVector, {component, count});
        vectorTypes[{component, count}] = id;
        return id;
    }

    static void append(std::vector<uint32_t>& to, const spv::Op op, const std::initializer_list<uint32_t> words) {
        to.push_back(static_cast<uint32_t>(words.size() + 1) << 16 | op);
        to.insert(to.end(), words);
    }

    static void append(std::vector<uint32_t>& to, const spv::Op op, const std::span<const uint32_t> words) {
        to.push_back(static_cast<uint32_t>(words.size() + 1) << 16 | op);
        to.insert(to.end(), words.begin(), words.end());
    }

    uint32_t global(const spv::Op op, const std::initializer_list<uint32_t> operands) {
        const uint32_t id = next();
        std::vector<uint32_t> words{id};
        words.insert(words.end(), operands);
        append(globals, op, words);
        return id;
    }

    static float finite(const float value) {
        if (std::isnan(value))
            return 0.0f;
        return std::clamp(value, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
    }

    uint32_t floatConstant(const float value) {
        const uint32_t bits = std::bit_cast<uint32_t>(finite(value));
        if (const auto found = floatConstants.find(bits); found != floatConstants.end())
            return found->second;
        const uint32_t id = next();
        append(globals, spv::OpConstant, {floatType, id, bits});
        floatConstants[bits] = id;
        return id;
    }

    uint32_t uintConstant(const uint32_t value) {
        if (const auto found = uintConstants.find(value); found != uintConstants.end())
            return found->second;
        const uint32_t id = next();
        append(globals, spv::OpConstant, {uintType, id, value});
        uintConstants[value] = id;
        return id;
    }

    uint32_t vectorConstant(const std::array<float, 4>& values) {
        const std::array<uint32_t, 4> lanes{floatConstant(values[0]), floatConstant(values[1]), floatConstant(values[2]), floatConstant(values[3])};
        if (const auto found = vectorConstants.find(lanes); found != vectorConstants.end())
            return found->second;
        const uint32_t id = next();
        append(globals, spv::OpConstantComposite, {float4Type, id, lanes[0], lanes[1], lanes[2], lanes[3]});
        vectorConstants[lanes] = id;
        return id;
    }

    uint32_t splatConstant(const float value) { return vectorConstant({value, value, value, value}); }

    uint32_t emit(const spv::Op op, const uint32_t type, const std::initializer_list<uint32_t> operands) {
        const uint32_t id = next();
        std::vector<uint32_t> words{type, id};
        words.insert(words.end(), operands);
        append(code, op, words);
        return id;
    }

    uint32_t extended(const GLSLstd450 instruction, const uint32_t type, const std::initializer_list<uint32_t> operands) {
        const uint32_t id = next();
        std::vector<uint32_t> words{type, id, glsl, static_cast<uint32_t>(instruction)};
        words.insert(words.end(), operands);
        append(code, spv::OpExtInst, words);
        return id;
    }

    uint32_t vector(const GLSLstd450 instruction, const std::initializer_list<uint32_t> operands) { return extended(instruction, float4Type, operands); }

    uint32_t call(const std::string_view name, const std::initializer_list<uint32_t> arguments, const std::initializer_list<uint32_t> parameterTypes,
                  const uint32_t resultType) {
        const std::optional<uint32_t> function = shell.function(name);
        if (!function) {
            fail(std::string("The material shell has no ") + std::string(name) + " function.");
            return zero;
        }
        const std::vector<uint32_t>& signature = shell.signature(*function);
        if (!std::ranges::equal(std::span(signature).subspan(1), parameterTypes) || signature[0] != resultType) {
            fail(std::string("The material shell function ") + std::string(name) + " has an unexpected signature.");
            return zero;
        }
        const uint32_t id = next();
        std::vector<uint32_t> words{resultType, id, *function};
        words.insert(words.end(), arguments);
        append(code, spv::OpFunctionCall, words);
        return id;
    }

    uint32_t helper(const std::string_view name, const std::initializer_list<uint32_t> arguments, const std::initializer_list<uint32_t> parameterTypes) {
        return call(name, arguments, parameterTypes, float4Type);
    }

    void fail(std::string message) {
        if (!error)
            error = std::move(message);
    }

    uint32_t lane(const uint32_t value, const uint32_t index) { return emit(spv::OpCompositeExtract, floatType, {value, index}); }

    uint32_t splat(const uint32_t scalar) { return emit(spv::OpCompositeConstruct, float4Type, {scalar, scalar, scalar, scalar}); }

    uint32_t masked(const uint32_t value, const uint32_t width) {
        if (width >= 4)
            return value;
        std::array<uint32_t, 4> components{};
        for (uint32_t i = 0; i < 4; ++i)
            components[i] = i < std::max(width, 1u) ? i : 4 + i;
        return emit(spv::OpVectorShuffle, float4Type, {value, zero, components[0], components[1], components[2], components[3]});
    }

    uint32_t widened(const uint32_t value, const uint32_t width) { return width == 1 ? splat(lane(value, 0)) : masked(value, width); }

    uint32_t binary(const spv::Op op, const uint32_t a, const uint32_t b) { return emit(op, float4Type, {a, b}); }

    uint32_t flag(const spv::Op compare, const uint32_t a, const uint32_t b) {
        return emit(spv::OpSelect, float4Type, {emit(compare, bool4Type, {a, b}), one, zero});
    }

    uint32_t width(const uint32_t index) const { return index < program.instructions.size() ? program.instructions[index].width : 1; }

    uint32_t value(const uint32_t index) const { return index < program.instructions.size() ? values[index] : zero; }

    std::array<float, 4> constantLanes(const ProgramInstruction& instruction) const {
        std::array<float, 4> lanes{};
        for (uint32_t i = 0; i < 4; ++i) {
            const std::size_t at = instruction.index + (instruction.width == 1 ? 0 : i);
            const bool inside = instruction.width == 1 || i < instruction.width;
            lanes[i] = inside && at < program.constants.size() ? finite(program.constants[at]) : 0.0f;
        }
        return lanes;
    }

    uint32_t constant(const uint32_t index) {
        const auto [slot, added] = constantSlots.try_emplace(index, static_cast<uint32_t>(constants.size()));
        if (added)
            constants.push_back(constantLanes(program.instructions[index]));
        return helper("materialConstant", {uintConstant(slot->second)}, {uintType});
    }

    uint32_t geometry(const uint32_t attribute, const Stage stage) {
        return helper(stage == Stage::Vertex ? "materialVertexGeometry" : "materialGeometry", {uintConstant(attribute)}, {uintType});
    }

    uint32_t combined(const ProgramInstruction& instruction) {
        const uint32_t third = instruction.width >= 3 ? lane(value(instruction.c), 0) : floatConstant(0.0f);
        return emit(spv::OpCompositeConstruct, float4Type, {lane(value(instruction.a), 0), lane(value(instruction.b), 0), third, floatConstant(0.0f)});
    }

    uint32_t power(const uint32_t base, const uint32_t exponent) {
        const uint32_t raised = vector(GLSLstd450Pow, {vector(GLSLstd450FMax, {base, splatConstant(1e-30f)}), exponent});
        return emit(spv::OpSelect, float4Type, {emit(spv::OpFOrdLessThanEqual, bool4Type, {base, zero}), zero, raised});
    }

    uint32_t modulo(const uint32_t a, const uint32_t b) {
        return binary(spv::OpFSub, a, binary(spv::OpFMul, b, vector(GLSLstd450Floor, {binary(spv::OpFDiv, a, b)})));
    }

    uint32_t smoothstep(const uint32_t low, const uint32_t high, const uint32_t input) {
        const uint32_t t = vector(GLSLstd450FClamp, {binary(spv::OpFDiv, binary(spv::OpFSub, input, low), binary(spv::OpFSub, high, low)), zero, one});
        return binary(spv::OpFMul, binary(spv::OpFMul, t, t), binary(spv::OpFSub, splatConstant(3.0f), binary(spv::OpFMul, splatConstant(2.0f), t)));
    }

    uint32_t random(const uint32_t seed) {
        const uint32_t dot = emit(spv::OpDot, floatType, {seed, vectorConstant({12.9898f, 78.233f, 37.719f, 4.581f})});
        const uint32_t sine = extended(GLSLstd450Sin, floatType, {dot});
        return splat(extended(GLSLstd450Fract, floatType, {emit(spv::OpFMul, floatType, {sine, floatConstant(43758.5453f)})}));
    }

    uint32_t normalized(const uint32_t value, const uint32_t width) {
        const uint32_t lanes = masked(value, width);
        const uint32_t length = extended(GLSLstd450FMax, floatType, {extended(GLSLstd450Length, floatType, {lanes}), floatConstant(1e-30f)});
        return binary(spv::OpFDiv, lanes, splat(length));
    }

    uint32_t cross(const uint32_t a, const uint32_t b) {
        const uint32_t left = emit(spv::OpVectorShuffle, float3Type, {a, a, 0, 1, 2});
        const uint32_t right = emit(spv::OpVectorShuffle, float3Type, {b, b, 0, 1, 2});
        const uint32_t product = extended(GLSLstd450Cross, float3Type, {left, right});
        return emit(spv::OpVectorShuffle, float4Type, {product, zero, 0, 1, 2, 6});
    }

    uint32_t expression(const uint32_t index, const Stage stage) {
        const ProgramInstruction& instruction = program.instructions[index];
        const uint32_t w = instruction.width;
        const uint32_t a = value(instruction.a);
        const uint32_t b = value(instruction.b);
        const uint32_t c = value(instruction.c);
        const uint32_t operandWidth = std::max(width(instruction.a), width(instruction.b));
        switch (instruction.op) {
        case ProgramOp::Constant:
            return constant(index);
        case ProgramOp::Parameter:
            if (instruction.index >= program.parameters.size())
                return zero;
            return helper("materialParameter", {uintConstant(instruction.index)}, {uintType});
        case ProgramOp::Texture:
            if (instruction.index >= program.textures.size())
                return zero;
            return helper(stage == Stage::Vertex ? "materialVertexTexture" : "materialTexture", {uintConstant(instruction.index), a}, {uintType, float4Type});
        case ProgramOp::Geometry:
            return geometry(instruction.index, stage);
        case ProgramOp::GeometryDx:
            if (stage == Stage::Vertex)
                return zero;
            return helper("materialGeometryDx", {uintConstant(instruction.index)}, {uintType});
        case ProgramOp::GeometryDy:
            if (stage == Stage::Vertex)
                return zero;
            return helper("materialGeometryDy", {uintConstant(instruction.index)}, {uintType});
        case ProgramOp::Time:
            return helper("materialTime", {}, {});
        case ProgramOp::Frame:
            return helper("materialFrame", {}, {});
        case ProgramOp::Extract:
            return splat(lane(a, std::min<uint32_t>(instruction.index, 3)));
        case ProgramOp::Combine:
            return combined(instruction);
        case ProgramOp::Add:
            return binary(spv::OpFAdd, a, b);
        case ProgramOp::Sub:
            return binary(spv::OpFSub, a, b);
        case ProgramOp::Mul:
            return binary(spv::OpFMul, a, b);
        case ProgramOp::Div:
            return binary(spv::OpFDiv, a, b);
        case ProgramOp::Min:
            return vector(GLSLstd450FMin, {a, b});
        case ProgramOp::Max:
            return vector(GLSLstd450FMax, {a, b});
        case ProgramOp::Pow:
            return power(a, b);
        case ProgramOp::Atan2:
            return vector(GLSLstd450Atan2, {a, b});
        case ProgramOp::Mod:
            return modulo(a, b);
        case ProgramOp::Abs:
            return vector(GLSLstd450FAbs, {a});
        case ProgramOp::Floor:
            return vector(GLSLstd450Floor, {a});
        case ProgramOp::Ceil:
            return vector(GLSLstd450Ceil, {a});
        case ProgramOp::Fract:
            return vector(GLSLstd450Fract, {a});
        case ProgramOp::Round:
            return vector(GLSLstd450RoundEven, {a});
        case ProgramOp::Trunc:
            return vector(GLSLstd450Trunc, {a});
        case ProgramOp::Sign:
            return vector(GLSLstd450FSign, {a});
        case ProgramOp::Sqrt:
            return vector(GLSLstd450Sqrt, {vector(GLSLstd450FMax, {a, zero})});
        case ProgramOp::Exp2:
            return vector(GLSLstd450Exp2, {a});
        case ProgramOp::Log2:
            return vector(GLSLstd450Log2, {a});
        case ProgramOp::Sin:
            return vector(GLSLstd450Sin, {a});
        case ProgramOp::Cos:
            return vector(GLSLstd450Cos, {a});
        case ProgramOp::Tan:
            return vector(GLSLstd450Tan, {a});
        case ProgramOp::Asin:
            return vector(GLSLstd450Asin, {vector(GLSLstd450FClamp, {a, splatConstant(-1.0f), one})});
        case ProgramOp::Acos:
            return vector(GLSLstd450Acos, {vector(GLSLstd450FClamp, {a, splatConstant(-1.0f), one})});
        case ProgramOp::Atan:
            return vector(GLSLstd450Atan, {a});
        case ProgramOp::Random:
            return random(widened(a, width(instruction.a)));
        case ProgramOp::Less:
            return flag(spv::OpFOrdLessThan, a, b);
        case ProgramOp::LessEqual:
            return flag(spv::OpFOrdLessThanEqual, a, b);
        case ProgramOp::Equal:
            return flag(spv::OpFOrdEqual, a, b);
        case ProgramOp::NotEqual:
            return flag(spv::OpFUnordNotEqual, a, b);
        case ProgramOp::Select:
            return emit(spv::OpSelect, float4Type, {emit(spv::OpFUnordNotEqual, bool4Type, {a, zero}), b, c});
        case ProgramOp::Dot:
            return splat(emit(spv::OpDot, floatType, {masked(a, operandWidth), masked(b, operandWidth)}));
        case ProgramOp::Cross:
            return cross(a, b);
        case ProgramOp::Length:
            return splat(extended(GLSLstd450Length, floatType, {masked(a, width(instruction.a))}));
        case ProgramOp::Distance:
            return splat(extended(GLSLstd450Distance, floatType, {masked(a, operandWidth), masked(b, operandWidth)}));
        case ProgramOp::Normalize:
            return normalized(a, w);
        case ProgramOp::Reflect:
            return vector(GLSLstd450Reflect, {masked(a, w), masked(b, w)});
        case ProgramOp::TangentToWorld:
            return helper("materialTangentToWorld", {a}, {float4Type});
        case ProgramOp::Clamp:
            return vector(GLSLstd450FMin, {vector(GLSLstd450FMax, {a, b}), c});
        case ProgramOp::Mix:
            return binary(spv::OpFAdd, a, binary(spv::OpFMul, binary(spv::OpFSub, b, a), c));
        case ProgramOp::Smoothstep:
            return smoothstep(b, c, a);
        case ProgramOp::MulAdd:
            return binary(spv::OpFAdd, binary(spv::OpFMul, a, b), c);
        }
        return zero;
    }

    void statements(const std::vector<uint32_t>& outputs, const Stage stage) {
        std::vector<bool> used(program.instructions.size(), false);
        std::vector<uint32_t> pending;
        for (const uint32_t output : outputs)
            if (output < program.instructions.size())
                pending.push_back(output);
        while (!pending.empty()) {
            const uint32_t index = pending.back();
            pending.pop_back();
            if (used[index])
                continue;
            used[index] = true;
            for (const uint32_t operand : operands(program.instructions[index]))
                if (operand < program.instructions.size())
                    pending.push_back(operand);
        }
        values.assign(program.instructions.size(), zero);
        for (uint32_t i = 0; i < program.instructions.size(); ++i) {
            if (!used[i])
                continue;
            const ProgramInstruction& instruction = program.instructions[i];
            uint32_t result = expression(i, stage);
            if (instruction.width == 1 && instruction.op != ProgramOp::Constant && instruction.op != ProgramOp::Parameter)
                result = splat(lane(result, 0));
            values[i] = result;
        }
    }

    std::vector<uint32_t> finish() {
        std::vector<uint32_t> body;
        body.swap(code);
        return body;
    }

    std::vector<uint32_t> writeSurface() {
        std::vector<uint32_t> outputs{program.baseColor, program.normal, program.emission, program.roughness, program.metallic, program.specular};
        for (const int32_t output : {program.opacity, program.refraction})
            if (output >= 0)
                outputs.push_back(static_cast<uint32_t>(output));
        statements(outputs, Stage::Fragment);
        const uint32_t opacity = program.opacity >= 0 ? value(static_cast<uint32_t>(program.opacity)) : one;
        const uint32_t refraction = program.refraction >= 0 ? value(static_cast<uint32_t>(program.refraction)) : zero;
        call("materialOutput",
             {value(program.baseColor), value(program.normal), value(program.emission), value(program.roughness), value(program.metallic),
              value(program.specular), opacity, refraction},
             {float4Type, float4Type, float4Type, float4Type, float4Type, float4Type, float4Type, float4Type}, voidType);
        return finish();
    }

    std::vector<uint32_t> writeDisplacement() {
        uint32_t displacement = zero;
        if (program.displacement >= 0 && static_cast<std::size_t>(program.displacement) < program.instructions.size()) {
            statements({static_cast<uint32_t>(program.displacement)}, Stage::Vertex);
            displacement = value(static_cast<uint32_t>(program.displacement));
        }
        call("materialDisplacementOutput", {displacement}, {float4Type}, voidType);
        return finish();
    }

    static std::vector<uint32_t> calledFunctions(const std::span<const uint32_t> body) {
        std::vector<uint32_t> called;
        for (std::size_t at = 0; at < body.size(); at += body[at] >> 16)
            if ((body[at] & 0xffff) == spv::OpFunctionCall)
                called.push_back(body[at + 3]);
        return called;
    }

    std::unordered_map<uint32_t, std::vector<uint32_t>> callGraph() const {
        std::unordered_map<uint32_t, std::vector<uint32_t>> calls;
        std::optional<uint32_t> inside;
        for (const Instruction& instruction : shell.instructions)
            if (instruction.op == spv::OpFunction)
                inside = instruction.operands[1];
            else if (instruction.op == spv::OpFunctionEnd)
                inside.reset();
            else if (inside && instruction.op == spv::OpFunctionCall && !bodies.contains(*inside))
                calls[*inside].push_back(instruction.operands[2]);
        for (const auto& [function, body] : bodies)
            calls[function] = calledFunctions(body);
        return calls;
    }

    static std::unordered_set<uint32_t> reachableFrom(const uint32_t entry, const std::unordered_map<uint32_t, std::vector<uint32_t>>& calls) {
        std::unordered_set<uint32_t> reached;
        std::vector<uint32_t> pending{entry};
        while (!pending.empty()) {
            const uint32_t function = pending.back();
            pending.pop_back();
            if (!reached.insert(function).second)
                continue;
            if (const auto found = calls.find(function); found != calls.end())
                pending.insert(pending.end(), found->second.begin(), found->second.end());
        }
        return reached;
    }

    std::unordered_set<uint32_t> keptFunctions() const {
        const std::unordered_map<uint32_t, std::vector<uint32_t>> calls = callGraph();
        std::unordered_set<uint32_t> kept;
        for (const Instruction& instruction : shell.instructions) {
            if (instruction.op != spv::OpEntryPoint)
                continue;
            std::unordered_set<uint32_t> reached = reachableFrom(instruction.operands[1], calls);
            if (std::ranges::any_of(materialFunctions, [&](const uint32_t function) { return reached.contains(function); }))
                kept.merge(reached);
        }
        return kept;
    }

    static bool namesTarget(const spv::Op op) {
        return op == spv::OpName || op == spv::OpMemberName || op == spv::OpDecorate || op == spv::OpMemberDecorate || op == spv::OpDecorateString;
    }

    static bool removesEntry(const Instruction& instruction, const std::unordered_set<uint32_t>& kept) {
        if (instruction.op == spv::OpEntryPoint)
            return !kept.contains(instruction.operands[1]);
        if (instruction.op == spv::OpExecutionMode || instruction.op == spv::OpExecutionModeId)
            return !kept.contains(instruction.operands[0]);
        return false;
    }

    std::vector<bool> removedIds(const std::unordered_set<uint32_t>& kept) const {
        std::vector<bool> ids(bound, false);
        std::vector<bool> referenced(bound, false);
        const auto reference = [&](const std::span<const uint32_t> operands) {
            for (const uint32_t operand : operands)
                if (operand < bound)
                    referenced[operand] = true;
        };
        std::vector<uint32_t> variables;
        std::optional<uint32_t> inside;
        for (const Instruction& instruction : shell.instructions) {
            if (instruction.op == spv::OpFunction) {
                inside = instruction.operands[1];
                if (!kept.contains(*inside))
                    ids[*inside] = true;
            } else if (instruction.op == spv::OpFunctionEnd) {
                inside.reset();
                continue;
            }
            const bool dropped = inside && (!kept.contains(*inside) || (bodies.contains(*inside) && instruction.op != spv::OpFunction));
            if (dropped) {
                if (const std::optional<uint32_t> id = resultId(instruction))
                    ids[*id] = true;
                continue;
            }
            if (namesTarget(instruction.op) || removesEntry(instruction, kept))
                continue;
            if (!inside && instruction.op == spv::OpVariable) {
                variables.push_back(instruction.operands[1]);
                referenced[instruction.operands[0]] = true;
                if (instruction.operands.size() > 3)
                    referenced[instruction.operands[3]] = true;
                continue;
            }
            reference(instruction.operands);
        }
        for (const auto& [function, body] : bodies)
            reference(body);
        reference(globals);
        for (const uint32_t variable : variables)
            if (!referenced[variable])
                ids[variable] = true;
        return ids;
    }

    std::vector<uint32_t> assemble(const std::unordered_set<uint32_t>& kept) {
        const std::vector<bool> removed = removedIds(kept);
        const auto isRemoved = [&](const uint32_t id) { return id < removed.size() && removed[id]; };
        std::vector<uint32_t> output(shell.words.begin(), shell.words.begin() + 5);
        bool globalsWritten = false;
        bool skipping = false;
        std::optional<uint32_t> replacing;
        for (const Instruction& instruction : shell.instructions) {
            if (replacing) {
                if (instruction.op != spv::OpFunctionEnd)
                    continue;
                append(output, spv::OpLabel, {next()});
                output.insert(output.end(), bodies.at(*replacing).begin(), bodies.at(*replacing).end());
                append(output, spv::OpReturn, {});
                append(output, spv::OpFunctionEnd, {});
                replacing.reset();
                continue;
            }
            if (skipping) {
                skipping = instruction.op != spv::OpFunctionEnd;
                continue;
            }
            if (namesTarget(instruction.op) && isRemoved(instruction.operands[0]))
                continue;
            if (removesEntry(instruction, kept))
                continue;
            if (instruction.op == spv::OpVariable && isRemoved(instruction.operands[1]))
                continue;
            if (instruction.op == spv::OpMemoryModel && !shell.glsl) {
                constexpr std::string_view name = "GLSL.std.450";
                std::vector<uint32_t> words{glsl};
                words.resize(1 + name.size() / 4 + 1, 0);
                std::memcpy(words.data() + 1, name.data(), name.size());
                append(output, spv::OpExtInstImport, words);
            }
            if (instruction.op == spv::OpFunction && !globalsWritten) {
                output.insert(output.end(), globals.begin(), globals.end());
                globalsWritten = true;
            }
            if (instruction.op == spv::OpFunction && !kept.contains(instruction.operands[1])) {
                skipping = true;
                continue;
            }
            append(output, instruction.op, instruction.operands);
            if (instruction.op == spv::OpFunction && bodies.contains(instruction.operands[1]))
                replacing = instruction.operands[1];
        }
        output[3] = bound;
        return output;
    }

    const MaterialShell::Module& shell;
    const MaterialProgram& program;
    uint32_t bound = 0;
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> vectorTypes;
    std::optional<std::string> error;
    uint32_t glsl = 0;
    uint32_t voidType = 0;
    uint32_t boolType = 0;
    uint32_t floatType = 0;
    uint32_t uintType = 0;
    uint32_t float3Type = 0;
    uint32_t float4Type = 0;
    uint32_t bool4Type = 0;
    uint32_t zero = 0;
    uint32_t one = 0;
    std::vector<uint32_t> globals;
    std::vector<uint32_t> code;
    std::vector<uint32_t> values;
    std::unordered_map<uint32_t, uint32_t> floatConstants;
    std::unordered_map<uint32_t, uint32_t> uintConstants;
    std::map<std::array<uint32_t, 4>, uint32_t> vectorConstants;
    std::map<uint32_t, std::vector<uint32_t>> bodies;
    std::vector<uint32_t> materialFunctions;
    std::unordered_map<uint32_t, uint32_t> constantSlots;
    std::vector<std::array<float, 4>> constants;
};

}

MaterialShell::MaterialShell(const std::span<const uint32_t> words) : module(std::make_unique<Module>(words)) {
    failure = module->read();
}

MaterialShell::~MaterialShell() = default;

ConversionResult<MaterialSpirv> MaterialShell::write(const MaterialProgram& program) const {
    if (failure)
        return conversionFailure(*failure);
    return Writer(*module, program).write();
}

}
