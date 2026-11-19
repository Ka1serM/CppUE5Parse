#include "Conversion/Materials/MaterialWgsl.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <format>
#include <limits>
#include <string_view>
#include <unordered_map>

namespace ue5parse::conversion {
namespace {

enum class Stage : uint8_t { Vertex, Fragment };

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

float finite(const float value) {
    if (std::isnan(value))
        return 0.0f;
    return std::clamp(value, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
}

std::string literal(const float value) {
    std::string text = std::format("{}", finite(value));
    if (text.find_first_of(".e") == std::string::npos)
        text += ".0";
    return text;
}

std::string unsignedLiteral(const uint32_t value) {
    return std::format("{}u", value);
}

std::string lane(const std::string_view value, const uint32_t index) {
    return std::format("{}.{}", value, "xyzw"[std::min(index, 3u)]);
}

class Writer {
public:
    explicit Writer(const MaterialProgram& program) : program(program) {}

    MaterialWgsl write() {
        writeSurface();
        writeDisplacement();
        return {.code = std::move(code), .constants = std::move(constants)};
    }

private:
    static constexpr std::string_view zero = "zero4";
    static constexpr std::string_view one = "one4";

    static std::string vector(const std::string_view components) { return std::format("vec4f({})", components); }

    static std::string select(const std::string_view condition, const std::string_view whenTrue, const std::string_view whenFalse) {
        return std::format("select({}, {}, {})", whenFalse, whenTrue, condition);
    }

    void line(const std::string_view statement) { code += std::format("    {};\n", statement); }

    std::string define(std::string name, const std::string_view expression) {
        line(std::format("let {} = {}", name, expression));
        return name;
    }

    void begin(const std::string_view function) {
        code += std::format("fn {}() {{\n", function);
        define(std::string(zero), vector("0.0"));
        define(std::string(one), vector("1.0"));
    }

    void end() { code += "}\n"; }

    uint32_t width(const uint32_t index) const { return index < program.instructions.size() ? program.instructions[index].width : 1; }

    std::string value(const uint32_t index) const { return index < program.instructions.size() ? values[index] : std::string(zero); }

    std::string masked(const std::string_view value, const uint32_t width) const {
        switch (std::max(width, 1u)) {
        case 1:
            return vector(std::format("{}, 0.0, 0.0, 0.0", lane(value, 0)));
        case 2:
            return vector(std::format("{}.xy, 0.0, 0.0", value));
        case 3:
            return vector(std::format("{}.xyz, 0.0", value));
        default:
            return std::string(value);
        }
    }

    std::string widened(const std::string_view value, const uint32_t width) const { return width == 1 ? vector(lane(value, 0)) : masked(value, width); }

    std::array<float, 4> constantLanes(const ProgramInstruction& instruction) const {
        std::array<float, 4> lanes{};
        for (uint32_t i = 0; i < 4; ++i) {
            const std::size_t at = instruction.index + (instruction.width == 1 ? 0 : i);
            const bool inside = instruction.width == 1 || i < instruction.width;
            lanes[i] = inside && at < program.constants.size() ? finite(program.constants[at]) : 0.0f;
        }
        return lanes;
    }

    std::string constant(const uint32_t index) {
        const auto [slot, added] = constantSlots.try_emplace(index, static_cast<uint32_t>(constants.size()));
        if (added)
            constants.push_back(constantLanes(program.instructions[index]));
        return std::format("materialConstant({})", unsignedLiteral(slot->second));
    }

    std::string power(const std::string_view base, const std::string_view exponent) const {
        const std::string raised = std::format("pow(max({}, {}), {})", base, vector("1e-30"), exponent);
        return select(std::format("{} <= {}", base, zero), zero, raised);
    }

    std::string flag(const std::string_view comparison, const std::string_view a, const std::string_view b) const {
        return select(std::format("{} {} {}", a, comparison, b), one, zero);
    }

    std::string random(const std::string_view seed) const {
        const std::string dot = std::format("dot({}, {})", seed, vector("12.9898, 78.233, 37.719, 4.581"));
        return vector(std::format("fract(sin({}) * 43758.5453)", dot));
    }

    std::string expression(const uint32_t index, const Stage stage) {
        const ProgramInstruction& instruction = program.instructions[index];
        const uint32_t w = instruction.width;
        const std::string a = value(instruction.a);
        const std::string b = value(instruction.b);
        const std::string c = value(instruction.c);
        const uint32_t operandWidth = std::max(width(instruction.a), width(instruction.b));
        switch (instruction.op) {
        case ProgramOp::Constant:
            return constant(index);
        case ProgramOp::Parameter:
            if (instruction.index >= program.parameters.size())
                return std::string(zero);
            return std::format("materialParameter({})", unsignedLiteral(instruction.index));
        case ProgramOp::Texture:
            if (instruction.index >= program.textures.size())
                return std::string(zero);
            return std::format("{}({}, {})", stage == Stage::Vertex ? "materialVertexTexture" : "materialTexture", unsignedLiteral(instruction.index), a);
        case ProgramOp::Geometry:
            return std::format("{}({})", stage == Stage::Vertex ? "materialVertexGeometry" : "materialGeometry", unsignedLiteral(instruction.index));
        case ProgramOp::GeometryDx:
            if (stage == Stage::Vertex)
                return std::string(zero);
            return std::format("materialGeometryDx({})", unsignedLiteral(instruction.index));
        case ProgramOp::GeometryDy:
            if (stage == Stage::Vertex)
                return std::string(zero);
            return std::format("materialGeometryDy({})", unsignedLiteral(instruction.index));
        case ProgramOp::Time:
            return "materialTime()";
        case ProgramOp::Frame:
            return "materialFrame()";
        case ProgramOp::Extract:
            return vector(lane(a, instruction.index));
        case ProgramOp::Combine:
            return vector(std::format("{}, {}, {}, 0.0", lane(a, 0), lane(b, 0), w >= 3 ? lane(c, 0) : "0.0"));
        case ProgramOp::Add:
            return std::format("{} + {}", a, b);
        case ProgramOp::Sub:
            return std::format("{} - {}", a, b);
        case ProgramOp::Mul:
            return std::format("{} * {}", a, b);
        case ProgramOp::Div:
            return std::format("{} / {}", a, b);
        case ProgramOp::Min:
            return std::format("min({}, {})", a, b);
        case ProgramOp::Max:
            return std::format("max({}, {})", a, b);
        case ProgramOp::Pow:
            return power(a, b);
        case ProgramOp::Atan2:
            return std::format("atan2({}, {})", a, b);
        case ProgramOp::Mod:
            return std::format("{} - {} * floor({} / {})", a, b, a, b);
        case ProgramOp::Abs:
            return std::format("abs({})", a);
        case ProgramOp::Floor:
            return std::format("floor({})", a);
        case ProgramOp::Ceil:
            return std::format("ceil({})", a);
        case ProgramOp::Fract:
            return std::format("fract({})", a);
        case ProgramOp::Round:
            return std::format("round({})", a);
        case ProgramOp::Trunc:
            return std::format("trunc({})", a);
        case ProgramOp::Sign:
            return std::format("sign({})", a);
        case ProgramOp::Sqrt:
            return std::format("sqrt(max({}, {}))", a, zero);
        case ProgramOp::Exp2:
            return std::format("exp2({})", a);
        case ProgramOp::Log2:
            return std::format("log2({})", a);
        case ProgramOp::Sin:
            return std::format("sin({})", a);
        case ProgramOp::Cos:
            return std::format("cos({})", a);
        case ProgramOp::Tan:
            return std::format("tan({})", a);
        case ProgramOp::Asin:
            return std::format("asin(clamp({}, -{}, {}))", a, one, one);
        case ProgramOp::Acos:
            return std::format("acos(clamp({}, -{}, {}))", a, one, one);
        case ProgramOp::Atan:
            return std::format("atan({})", a);
        case ProgramOp::Random:
            return random(widened(a, width(instruction.a)));
        case ProgramOp::Less:
            return flag("<", a, b);
        case ProgramOp::LessEqual:
            return flag("<=", a, b);
        case ProgramOp::Equal:
            return flag("==", a, b);
        case ProgramOp::NotEqual:
            return flag("!=", a, b);
        case ProgramOp::Select:
            return select(std::format("{} != {}", a, zero), b, c);
        case ProgramOp::Dot:
            return vector(std::format("dot({}, {})", masked(a, operandWidth), masked(b, operandWidth)));
        case ProgramOp::Cross:
            return vector(std::format("cross({}.xyz, {}.xyz), 0.0", a, b));
        case ProgramOp::Length:
            return vector(std::format("length({})", masked(a, width(instruction.a))));
        case ProgramOp::Distance:
            return vector(std::format("distance({}, {})", masked(a, operandWidth), masked(b, operandWidth)));
        case ProgramOp::Normalize: {
            const std::string lanes = define(std::format("n{}", index), masked(a, w));
            return std::format("{} / {}", lanes, vector(std::format("max(length({}), 1e-30)", lanes)));
        }
        case ProgramOp::Reflect:
            return std::format("reflect({}, {})", masked(a, w), masked(b, w));
        case ProgramOp::TangentToWorld:
            return std::format("materialTangentToWorld({})", a);
        case ProgramOp::Clamp:
            return std::format("min(max({}, {}), {})", a, b, c);
        case ProgramOp::Mix:
            return std::format("{} + ({} - {}) * {}", a, b, a, c);
        case ProgramOp::Smoothstep: {
            const std::string t = define(std::format("s{}", index), std::format("clamp(({} - {}) / ({} - {}), {}, {})", a, b, c, b, zero, one));
            return std::format("{} * {} * ({} - {} * {})", t, t, vector("3.0"), vector("2.0"), t);
        }
        case ProgramOp::MulAdd:
            return std::format("{} * {} + {}", a, b, c);
        }
        return std::string(zero);
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
        values.assign(program.instructions.size(), std::string(zero));
        for (uint32_t i = 0; i < program.instructions.size(); ++i) {
            if (!used[i])
                continue;
            const ProgramInstruction& instruction = program.instructions[i];
            std::string result = expression(i, stage);
            if (instruction.width == 1 && instruction.op != ProgramOp::Constant && instruction.op != ProgramOp::Parameter)
                result = vector(lane(std::format("({})", result), 0));
            values[i] = define(std::format("v{}", i), result);
        }
    }

    void writeSurface() {
        std::vector<uint32_t> outputs{program.baseColor, program.normal, program.emission, program.roughness, program.metallic, program.specular};
        for (const int32_t output : {program.opacity, program.refraction})
            if (output >= 0)
                outputs.push_back(static_cast<uint32_t>(output));
        begin("evaluateMaterial");
        statements(outputs, Stage::Fragment);
        const std::string opacity = program.opacity >= 0 ? value(static_cast<uint32_t>(program.opacity)) : std::string(one);
        const std::string refraction = program.refraction >= 0 ? value(static_cast<uint32_t>(program.refraction)) : std::string(zero);
        line(std::format("materialOutput({}, {}, {}, {}, {}, {}, {}, {})", value(program.baseColor), value(program.normal), value(program.emission),
                         value(program.roughness), value(program.metallic), value(program.specular), opacity, refraction));
        end();
    }

    void writeDisplacement() {
        begin("evaluateDisplacement");
        std::string displacement(zero);
        if (program.displacement >= 0 && static_cast<std::size_t>(program.displacement) < program.instructions.size()) {
            statements({static_cast<uint32_t>(program.displacement)}, Stage::Vertex);
            displacement = value(static_cast<uint32_t>(program.displacement));
        }
        line(std::format("materialDisplacementOutput({})", displacement));
        end();
    }

    const MaterialProgram& program;
    std::string code;
    std::vector<std::string> values;
    std::unordered_map<uint32_t, uint32_t> constantSlots;
    std::vector<std::array<float, 4>> constants;
};

}

MaterialWgsl writeMaterialWgsl(const MaterialProgram& program) {
    return Writer(program).write();
}

}
