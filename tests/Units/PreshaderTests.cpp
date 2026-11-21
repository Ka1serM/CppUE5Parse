#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Conversion/Materials/Preshader.h"
#include "Engine/Source/Runtime/Engine/Shader/Preshader.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::conversion::ExpressionGraph;
using ue5parse::conversion::ExprId;
using ue5parse::conversion::MaterialData;
using ue5parse::conversion::NumericParameter;
using ue5parse::conversion::PreshaderField;
using ue5parse::conversion::PreshaderProgram;
using ue5parse::conversion::Warnings;
using ue5parse::runtime::EPreshaderOpcode;
using ue5parse::runtime::FindPreshaderOpcode;

//! Writes preshader bytecode the way FMaterialPreshaderData does for the opcode set of a game.
class Program {
public:
    explicit Program(const EGame game) : game(game) {}

    Program& op(const EPreshaderOpcode opcode) {
        const std::optional<uint8_t> raw = FindPreshaderOpcode(game, opcode);
        if (!CHECK(raw.has_value()))
            return *this;
        bytes.push_back(*raw);
        return *this;
    }

    Program& constant(std::initializer_list<float> components) {
        op(EPreshaderOpcode::Constant);
        if (game < EGame::GAME_UE5_0) {
            for (std::size_t c = 0; c < 4; ++c)
                write(c < components.size() ? components.begin()[c] : 0.0f);
            return *this;
        }
        bytes.push_back(static_cast<uint8_t>(components.size()));
        for (const float component : components)
            write(component);
        return *this;
    }

    Program& zero(const uint8_t valueType) {
        op(EPreshaderOpcode::ConstantZero);
        if (game >= EGame::GAME_UE5_1)
            bytes.push_back(valueType);
        return *this;
    }

    Program& parameter(const EPreshaderOpcode opcode, const uint16_t index) {
        op(opcode);
        bytes.push_back(static_cast<uint8_t>(index & 0xffu));
        bytes.push_back(static_cast<uint8_t>(index >> 8));
        return *this;
    }

    Program& legacy(const EPreshaderOpcode opcode, const uint8_t operand) {
        op(opcode);
        if (game < EGame::GAME_UE5_0)
            bytes.push_back(operand);
        return *this;
    }

    Program& swizzle(std::initializer_list<uint8_t> indices) {
        op(EPreshaderOpcode::ComponentSwizzle);
        bytes.push_back(static_cast<uint8_t>(indices.size()));
        for (std::size_t i = 0; i < 4; ++i)
            bytes.push_back(i < indices.size() ? indices.begin()[i] : 0u);
        return *this;
    }

    //! Runs the program for one field of the given component count and returns its words with the parameters at their values.
    std::vector<float> run(const uint32_t components, const std::vector<NumericParameter>& parameters = {}) const {
        const PreshaderProgram program{0u, static_cast<uint32_t>(bytes.size()), 0u, 1u};
        const PreshaderField field{0u, 0u, static_cast<int>(components)};
        MaterialData data;
        data.game = game;
        data.parameters = parameters;
        data.preshaders = {&program, 1};
        data.fields = {&field, 1};
        data.preshaderCode = bytes;
        ExpressionGraph graph;
        Warnings warnings;
        const std::vector<ExprId> words = ue5parse::conversion::materialUniforms(graph, data, warnings);
        CHECK(warnings.getMessages().empty());
        std::unordered_map<ExprId, ExprId> values;
        for (uint32_t index = 0; index < parameters.size(); ++index)
            for (uint32_t component = 0; component < 4; ++component)
                values.emplace(graph.parameter(index, component), graph.constant(parameters[index].value[component]));
        std::vector<float> result;
        for (const ExprId word : words) {
            const ExprId value = graph.substitute(word, values);
            if (!CHECK(graph.isConstant(value)))
                return {};
            result.push_back(graph.node(value).value);
        }
        return result;
    }

private:
    void write(const float value) {
        uint8_t raw[sizeof(float)];
        std::memcpy(raw, &value, sizeof(float));
        bytes.insert(bytes.end(), raw, raw + sizeof(float));
    }

    EGame game;
    std::vector<uint8_t> bytes;
};

bool near(const std::vector<float>& actual, std::initializer_list<float> expected) {
    if (actual.size() != expected.size())
        return false;
    for (std::size_t i = 0; i < actual.size(); ++i)
        if (std::abs(actual[i] - expected.begin()[i]) > 1e-4f * std::max(1.0f, std::abs(expected.begin()[i])))
            return false;
    return true;
}

constexpr EGame ue4 = EGame::GAME_UE4_25;
constexpr EGame ue5 = EGame::GAME_UE5_8;

}

TEST(preshaderArithmeticUE5) {
    using enum EPreshaderOpcode;
    CHECK(near(Program(ue5).constant({6.0f}).constant({3.0f}).op(Div).run(1), {2.0f}));
    CHECK(near(Program(ue5).constant({2.0f}).constant({3.0f}).op(Sub).run(1), {-1.0f}));
    CHECK(near(Program(ue5).constant({7.0f}).constant({3.0f}).op(Fmod).run(1), {1.0f}));
    CHECK(near(Program(ue5).constant({-7.0f}).constant({3.0f}).op(Fmod).run(1), {-1.0f}));
    CHECK(near(Program(ue5).constant({5.0f}).constant({0.0f}).constant({2.0f}).op(Clamp).run(1), {2.0f}));
    CHECK(near(Program(ue5).constant({-1.0f}).op(Saturate).run(1), {0.0f}));
    CHECK(near(Program(ue5).constant({3.0f}).op(Neg).run(1), {-3.0f}));
    CHECK(near(Program(ue5).constant({0.0f}).op(Exp).run(1), {1.0f}));
    CHECK(near(Program(ue5).constant({100.0f}).op(Log10).run(1), {2.0f}));
    CHECK(near(Program(ue5).constant({-1.25f}).op(Fractional).run(1), {-0.25f}));
}

TEST(preshaderSafeDivisor) {
    using enum EPreshaderOpcode;
    for (const EGame game : {ue4, ue5}) {
        CHECK(near(Program(game).constant({1.0f}).constant({0.0f}).op(Div).run(1), {100000.0f}));
        CHECK(near(Program(game).constant({1.0f}).constant({-0.000001f}).op(Div).run(1), {-100000.0f}));
    }
    CHECK(near(Program(ue5).constant({0.0f}).op(Rcp).run(1), {100000.0f}));
    CHECK(near(Program(ue5).constant({4.0f}).op(Rcp).run(1), {0.25f}));
}

TEST(preshaderVectorsUE5) {
    using enum EPreshaderOpcode;
    CHECK(near(Program(ue5).constant({1.0f, 2.0f, 3.0f}).constant({4.0f, 5.0f, 6.0f}).op(Dot).run(1), {32.0f}));
    CHECK(near(Program(ue5).constant({1.0f, 0.0f, 0.0f}).constant({0.0f, 1.0f, 0.0f}).op(Cross).run(3), {0.0f, 0.0f, 1.0f}));
    CHECK(near(Program(ue5).constant({3.0f, 4.0f}).op(Length).run(1), {5.0f}));
    CHECK(near(Program(ue5).constant({3.0f, 0.0f, 4.0f}).op(Normalize).run(3), {0.6f, 0.0f, 0.8f}));
    CHECK(near(Program(ue5).constant({1.0f}).constant({2.0f, 3.0f}).op(AppendVector).run(3), {1.0f, 2.0f, 3.0f}));
    CHECK(near(Program(ue5).constant({1.0f, 2.0f, 3.0f, 4.0f}).swizzle({3, 0}).run(2), {4.0f, 1.0f}));
    CHECK(near(Program(ue5).zero(2).constant({1.0f, 1.0f}).op(Add).run(2), {1.0f, 1.0f}));
}

TEST(preshaderParametersUE5) {
    using enum EPreshaderOpcode;
    const std::vector<NumericParameter> parameters{{"Scale", false, {0.25f}}, {"Tint", true, {1.0f, 2.0f, 3.0f, 4.0f}}};
    CHECK(near(Program(ue5).parameter(Parameter, 0).constant({4.0f}).op(Mul).run(1, parameters), {1.0f}));
    CHECK(near(Program(ue5).parameter(Parameter, 1).swizzle({2}).run(1, parameters), {3.0f}));
}

TEST(preshaderLegacyOperandsUE4) {
    using enum EPreshaderOpcode;
    //! EMaterialValueType MCT_Float3 is 4, so Dot drops the fourth component of both FLinearColors.
    CHECK(near(Program(ue4).constant({1.0f, 2.0f, 3.0f, 9.0f}).constant({4.0f, 5.0f, 6.0f, 9.0f}).legacy(Dot, 4).run(1), {32.0f}));
    CHECK(near(Program(ue4).constant({1.0f, 0.0f, 0.0f}).constant({0.0f, 1.0f, 0.0f}).legacy(Cross, 4).run(4), {0.0f, 0.0f, 1.0f, 0.0f}));
    CHECK(near(Program(ue4).constant({1.0f, 7.0f}).constant({2.0f, 3.0f}).legacy(AppendVector, 1).run(4), {1.0f, 2.0f, 3.0f, 0.0f}));
    const std::vector<NumericParameter> parameters{{"Scale", false, {0.5f}}, {"Tint", true, {1.0f, 2.0f, 3.0f, 4.0f}}};
    CHECK(near(Program(ue4).parameter(ScalarParameter, 0).constant({3.0f}).op(Mul).run(1, parameters), {1.5f}));
    //! UE4 numbers the vector parameters after all scalar parameters.
    CHECK(near(Program(ue4).parameter(VectorParameter, 0).swizzle({1}).run(1, parameters), {2.0f}));
}

TEST(preshaderOpcodeSets) {
    using enum EPreshaderOpcode;
    CHECK(FindPreshaderOpcode(ue4, ScalarParameter) == 3u && FindPreshaderOpcode(ue4, VectorParameter) == 4u);
    CHECK(!FindPreshaderOpcode(ue4, Parameter).has_value() && !FindPreshaderOpcode(ue4, Rcp).has_value());
    CHECK(FindPreshaderOpcode(ue5, Parameter).has_value() && !FindPreshaderOpcode(ue5, ScalarParameter).has_value());
}
