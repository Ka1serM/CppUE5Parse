#include "Conversion/Materials/Preshader.h"

#include <algorithm>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include "Engine/Source/Runtime/Engine/Shader/Preshader.h"

namespace ue5parse::conversion {
namespace {

using Vector = std::vector<ExprId>;
using Stack = std::vector<Vector>;
using enum runtime::EPreshaderOpcode;

class Preshader {
public:
    Preshader(ExpressionGraph& graph, const MaterialData& data, Warnings& warnings) : graph(graph), data(data), warnings(warnings), code(data.preshaderCode) {}

    std::vector<ExprId> buffer() {
        std::vector<ExprId> words;
        for (const PreshaderProgram& preshader : data.preshaders) {
            Vector value = runProgram(preshader);
            if (failure) {
                warnings.add(*failure + "; the words that program fills read as zero");
                failure.reset();
                value.clear();
            }
            if (size_t(preshader.fieldIndex) + preshader.fieldCount > data.fields.size()) {
                warnings.add("a preshader program fills fields that the material does not have");
                continue;
            }
            for (unsigned f = preshader.fieldIndex; f < preshader.fieldIndex + preshader.fieldCount; ++f) {
                const PreshaderField& field = data.fields[f];
                const uint32_t components = runtime::GetPreshaderValueComponents(static_cast<uint8_t>(field.type));
                if (components == 0) {
                    warnings.add("unsupported preshader field type " + std::to_string(field.type) + "; its words read as zero");
                    continue;
                }
                if (words.size() < field.bufferOffset + components)
                    words.resize(field.bufferOffset + components, graph.constant(0.0f));
                for (uint32_t k = 0; k < components; ++k)
                    words[field.bufferOffset + k] = value.empty() ? graph.constant(0.0f) : component(value, field.componentIndex + k);
            }
        }
        return words;
    }

private:
    Vector runProgram(const PreshaderProgram& preshader) {
        if (preshader.opcodeOffset > code.size() || preshader.opcodeSize > code.size() - preshader.opcodeOffset) {
            fail("a preshader program lies outside the preshader code");
            return {};
        }
        return run(code.subspan(preshader.opcodeOffset, preshader.opcodeSize));
    }

    void fail(std::string message) {
        if (!failure)
            failure = std::move(message);
    }

    ExprId parameter(const uint32_t index, const uint32_t component) {
        const NumericParameter& numeric = data.parameters[index];
        if (numeric.name == "SelectionColor")
            return graph.constant(numeric.value[component]);
        return graph.parameter(index, component);
    }

    Vector pop(Stack& stack) {
        if (stack.empty()) {
            fail("preshader stack underflow");
            return {graph.constant(0.0f)};
        }
        Vector value = std::move(stack.back());
        stack.pop_back();
        return value;
    }

    ExprId component(const Vector& value, const size_t index) {
        if (value.empty()) {
            fail("empty preshader value");
            return graph.constant(0.0f);
        }
        return index < value.size() ? value[index] : value.back();
    }

    template <class T>
    std::optional<T> checked(ue5parse::Result<T> value) {
        if (value)
            return std::move(*value);
        fail("malformed preshader: " + std::string(ue5parse::getErrorName(value.error().code)) + " at " + value.error().where);
        return std::nullopt;
    }

    Vector map(const Vector& a, const ExprOp op) {
        Vector r;
        for (const ExprId x : a)
            r.push_back(graph.unary(op, x));
        return r;
    }
    Vector map(const Vector& a, const Vector& b, const ExprOp op) {
        const size_t count = std::max(a.size(), b.size());
        Vector r;
        for (size_t i = 0; i < count; ++i)
            r.push_back(graph.binary(op, component(a, a.size() == 1 ? 0 : i), component(b, b.size() == 1 ? 0 : i)));
        return r;
    }
    ExprId dot(const Vector& a, const Vector& b) {
        ExprId sum = graph.constant(0.0f);
        for (size_t i = 0; i < std::min(a.size(), b.size()); ++i)
            sum = graph.add(sum, graph.mul(a[i], b[i]));
        return sum;
    }

    ExprId truncatedRemainder(const ExprId a, const ExprId b) { return graph.sub(a, graph.mul(b, graph.unary(ExprOp::Trunc, graph.div(a, b)))); }

    bool arithmetic(const runtime::EPreshaderOpcode opcode, Stack& stack) {
        static constexpr std::pair<runtime::EPreshaderOpcode, ExprOp> binaries[] = {
            {Add, ExprOp::Add}, {Sub, ExprOp::Sub}, {Mul, ExprOp::Mul}, {Div, ExprOp::Div}, {Min, ExprOp::Min}, {Max, ExprOp::Max}, {Atan2, ExprOp::Atan2},
        };
        static constexpr std::pair<runtime::EPreshaderOpcode, ExprOp> unaries[] = {
            {Sin, ExprOp::Sin},   {Cos, ExprOp::Cos},    {Tan, ExprOp::Tan},     {Asin, ExprOp::Asin}, {Acos, ExprOp::Acos},   {Atan, ExprOp::Atan},
            {Sqrt, ExprOp::Sqrt}, {Abs, ExprOp::Abs},    {Floor, ExprOp::Floor}, {Ceil, ExprOp::Ceil}, {Round, ExprOp::Round}, {Trunc, ExprOp::Trunc},
            {Sign, ExprOp::Sign}, {Frac, ExprOp::Fract}, {Exp2, ExprOp::Exp2},   {Log2, ExprOp::Log2},
        };
        if (const auto binary = std::ranges::find(binaries, opcode, &std::pair<runtime::EPreshaderOpcode, ExprOp>::first); binary != std::end(binaries)) {
            const Vector b = pop(stack), a = pop(stack);
            stack.push_back(map(a, b, binary->second));
            return true;
        }
        if (const auto unary = std::ranges::find(unaries, opcode, &std::pair<runtime::EPreshaderOpcode, ExprOp>::first); unary != std::end(unaries)) {
            stack.push_back(map(pop(stack), unary->second));
            return true;
        }
        const auto scale = [&](const Vector& a, const float factor) { return map(a, Vector{graph.constant(factor)}, ExprOp::Mul); };
        switch (opcode) {
        case Fmod:
        case Modulo: {
            const Vector b = pop(stack), a = pop(stack);
            Vector result;
            for (size_t i = 0; i < std::max(a.size(), b.size()); ++i)
                result.push_back(truncatedRemainder(component(a, a.size() == 1 ? 0 : i), component(b, b.size() == 1 ? 0 : i)));
            stack.push_back(std::move(result));
            return true;
        }
        case Clamp: {
            const Vector high = pop(stack), low = pop(stack), x = pop(stack);
            stack.push_back(map(map(x, low, ExprOp::Max), high, ExprOp::Min));
            return true;
        }
        case Saturate:
            stack.push_back(map(map(pop(stack), Vector{graph.constant(0.0f)}, ExprOp::Max), Vector{graph.constant(1.0f)}, ExprOp::Min));
            return true;
        case Fractional: {
            Vector result;
            for (const ExprId x : pop(stack))
                result.push_back(truncatedRemainder(x, graph.constant(1.0f)));
            stack.push_back(std::move(result));
            return true;
        }
        case Exp:
            stack.push_back(map(scale(pop(stack), std::numbers::log2e_v<float>), ExprOp::Exp2));
            return true;
        case Log:
            stack.push_back(scale(map(pop(stack), ExprOp::Log2), std::numbers::ln2_v<float>));
            return true;
        case Log10:
            stack.push_back(scale(map(pop(stack), ExprOp::Log2), std::numbers::ln2_v<float> / std::numbers::ln10_v<float>));
            return true;
        case Neg:
            stack.push_back(scale(pop(stack), -1.0f));
            return true;
        default:
            return false;
        }
    }

    Vector run(const std::span<const uint8_t> program) {
        Stack stack;
        const auto take = [&] { return pop(stack); };
        runtime::FPreshaderReader reader(program, data.game);
        while (!reader.IsAtEnd() && !failure) {
            const std::optional<runtime::EPreshaderOpcode> opcode = checked(reader.ReadOpcode());
            if (!opcode || arithmetic(*opcode, stack))
                continue;
            switch (*opcode) {
            case ConstantZero:
                if (const std::optional<uint32_t> count = checked(reader.ReadZeroComponents()))
                    stack.push_back(Vector(*count, graph.constant(0.0f)));
                break;
            case Constant: {
                const std::optional<runtime::FPreshaderConstant> constant = checked(reader.ReadConstant());
                if (!constant)
                    break;
                Vector value;
                for (uint8_t c = 0; c < constant->NumComponents; ++c)
                    value.push_back(graph.constant(static_cast<float>(constant->Components[c])));
                stack.push_back(std::move(value));
                break;
            }
            case Parameter: {
                const std::optional<uint16_t> index = checked(reader.ReadParameterIndex());
                if (!index)
                    break;
                if (*index >= data.parameters.size()) {
                    fail("preshader parameter out of range");
                    break;
                }
                const uint32_t count = data.parameters[*index].vector ? 4 : 1;
                Vector value;
                for (uint32_t c = 0; c < count; ++c)
                    value.push_back(parameter(*index, c));
                stack.push_back(std::move(value));
                break;
            }
            case Dot: {
                const Vector b = take(), a = take();
                stack.push_back({dot(a, b)});
                break;
            }
            case Cross: {
                const Vector b = take(), a = take();
                const auto term = [&](int i, int j) {
                    return graph.sub(graph.mul(component(a, i), component(b, j)), graph.mul(component(a, j), component(b, i)));
                };
                stack.push_back({term(1, 2), term(2, 0), term(0, 1)});
                break;
            }
            case Rcp:
                stack.push_back(map(Vector{graph.constant(1.0f)}, take(), ExprOp::Div));
                break;
            case Length: {
                const Vector a = take();
                stack.push_back({graph.unary(ExprOp::Sqrt, dot(a, a))});
                break;
            }
            case Normalize: {
                const Vector a = take();
                stack.push_back(map(a, Vector{graph.unary(ExprOp::Sqrt, dot(a, a))}, ExprOp::Div));
                break;
            }
            case ComponentSwizzle: {
                const std::optional<runtime::FPreshaderSwizzle> swizzle = checked(reader.ReadSwizzle());
                if (!swizzle)
                    break;
                const Vector a = take();
                Vector result;
                for (uint8_t i = 0; i < swizzle->NumComponents && i < 4; ++i)
                    result.push_back(component(a, swizzle->ComponentIndices[i]));
                stack.push_back(std::move(result));
                break;
            }
            case AppendVector: {
                const Vector b = take();
                Vector a = take();
                a.insert(a.end(), b.begin(), b.end());
                stack.push_back(std::move(a));
                break;
            }
            case TextureSize:
            case TexelSize:
                stack.push_back(textureSize(reader, *opcode == TexelSize));
                break;
            default:
                fail("unsupported preshader opcode " + std::to_string(reader.GetRawOpcode()));
            }
        }
        if (failure)
            return {};
        if (stack.size() != 1) {
            fail("preshader left an unbalanced stack");
            return {};
        }
        return stack.back();
    }

    Vector textureSize(runtime::FPreshaderReader& reader, const bool texel) {
        const std::optional<runtime::FPreshaderTextureParameter> texture = checked(reader.ReadTextureParameter());
        if (!texture)
            return {graph.constant(0.0f)};
        if (texture->NameIndex >= data.preshaderNames.size()) {
            fail("preshader name out of range");
            return {graph.constant(0.0f)};
        }
        const std::string_view name = data.preshaderNames[texture->NameIndex];
        const std::span sizes = data.textureSizes;
        auto found = std::ranges::find_if(sizes, [&](const TextureExtent& size) { return name != "None" && name == size.name; });
        if (found == sizes.end())
            found = std::ranges::find_if(sizes, [&](const TextureExtent& size) { return size.name.empty() && size.index == texture->TextureIndex; });
        if (found == sizes.end()) {
            fail("no texture for preshader texture size " + std::string(name));
            return {graph.constant(0.0f)};
        }
        const float extent[] = {static_cast<float>(found->width), static_cast<float>(found->height), static_cast<float>(found->depth)};
        Vector value;
        for (const float e : extent)
            value.push_back(graph.constant(!texel ? e : e > 0.0f ? 1.0f / e : 0.0f));
        return value;
    }

    ExpressionGraph& graph;
    const MaterialData& data;
    Warnings& warnings;
    std::span<const uint8_t> code;
    std::optional<std::string> failure;
};

}

std::vector<ExprId> materialUniforms(ExpressionGraph& graph, const MaterialData& data, Warnings& warnings) {
    return Preshader(graph, data, warnings).buffer();
}

}
