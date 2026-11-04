#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <spirv/unified1/spirv.hpp>

#include "Conversion/Spirv/ExpressionGraph.h"
#include "Conversion/Spirv/ProgramWriter.h"
#include "Conversion/Spirv/SpirvEvaluator.h"
#include "Test.h"

using namespace ue5parse::conversion;

namespace {

class NormalInputs final : public ShaderInputs {
public:
    explicit NormalInputs(ExpressionGraph& graph) : graph(graph) {}

    ExprId constantBuffer(uint32_t, uint32_t) override { return graph.unresolved("constant buffer"); }
    ExprId input(uint32_t, const uint32_t component) override { return graph.geometry(GeometryAttribute::Normal, component); }
    ExprId frontFacing() override { return graph.boolean(true); }
    ExprId fragCoord(uint32_t) override { return graph.unresolved("frag coord"); }
    std::array<ExprId, 4> sample(uint32_t, std::span<const ExprId>) override { return unresolvedTexture(); }
    std::array<ExprId, 4> fetch(uint32_t, ExprId) override { return unresolvedTexture(); }
    std::array<ExprId, 3> textureSize(uint32_t, ExprId) override { return {graph.constant(1.0f), graph.constant(1.0f), graph.constant(1.0f)}; }

private:
    std::array<ExprId, 4> unresolvedTexture() {
        const ExprId value = graph.unresolved("texture");
        return {value, value, value, value};
    }

    ExpressionGraph& graph;
};

void emit(std::vector<uint32_t>& words, const spv::Op op, const std::initializer_list<uint32_t> operands) {
    words.push_back(static_cast<uint32_t>(operands.size() + 1) << 16 | op);
    words.insert(words.end(), operands);
}

std::vector<uint32_t> doubledNormalShader() {
    enum : uint32_t { Void = 1, MainType, Float, Vec3, Vec4, InputPointer, OutputPointer, In, Out, Two, One, Main, Entry, Loaded, Doubled, Color };
    const uint32_t main = 0x6e69616d;
    std::vector<uint32_t> words{spv::MagicNumber, 0x10000, 0, 32, 0};
    emit(words, spv::OpCapability, {spv::CapabilityShader});
    emit(words, spv::OpMemoryModel, {spv::AddressingModelLogical, spv::MemoryModelGLSL450});
    emit(words, spv::OpEntryPoint, {spv::ExecutionModelFragment, Main, main, 0, In, Out});
    emit(words, spv::OpDecorate, {In, spv::DecorationLocation, 0});
    emit(words, spv::OpDecorate, {Out, spv::DecorationLocation, 0});
    emit(words, spv::OpTypeVoid, {Void});
    emit(words, spv::OpTypeFunction, {MainType, Void});
    emit(words, spv::OpTypeFloat, {Float, 32});
    emit(words, spv::OpTypeVector, {Vec3, Float, 3});
    emit(words, spv::OpTypeVector, {Vec4, Float, 4});
    emit(words, spv::OpTypePointer, {InputPointer, spv::StorageClassInput, Vec3});
    emit(words, spv::OpTypePointer, {OutputPointer, spv::StorageClassOutput, Vec4});
    emit(words, spv::OpVariable, {InputPointer, In, spv::StorageClassInput});
    emit(words, spv::OpVariable, {OutputPointer, Out, spv::StorageClassOutput});
    emit(words, spv::OpConstant, {Float, Two, 0x40000000});
    emit(words, spv::OpConstant, {Float, One, 0x3f800000});
    emit(words, spv::OpFunction, {Void, Main, spv::FunctionControlMaskNone, MainType});
    emit(words, spv::OpLabel, {Entry});
    emit(words, spv::OpLoad, {Vec3, Loaded, In});
    emit(words, spv::OpVectorTimesScalar, {Vec3, Doubled, Loaded, Two});
    emit(words, spv::OpCompositeConstruct, {Vec4, Color, Doubled, One});
    emit(words, spv::OpStore, {Out, Color});
    emit(words, spv::OpReturn, {});
    emit(words, spv::OpFunctionEnd, {});
    return words;
}

}

TEST(spirvBecomesAVectorizedProgram) {
    const std::vector<uint32_t> shader = doubledNormalShader();
    ExpressionGraph graph;
    NormalInputs inputs(graph);
    const ShaderOutputs outputs = evaluateSpirv(shader, graph, inputs);
    REQUIRE(!graph.isFailed());
    REQUIRE(outputs.targets.size() == 1);

    const std::array<ExprId, 3> color{outputs.targets[0][0], outputs.targets[0][1], outputs.targets[0][2]};
    ProgramWriter writer(graph, {});
    const uint32_t colorOutput = writer.write(std::span<const ExprId, 3>(color));
    const uint32_t alphaOutput = writer.write(outputs.targets[0][3]);
    REQUIRE(!writer.getFailure());
    const ExpressionProgram program = writer.finish();

    const ProgramInstruction& doubled = program.instructions[program.outputs[colorOutput]];
    CHECK(doubled.op == ProgramOp::Mul);
    CHECK(doubled.width == 3);
    const ProgramInstruction& normal = program.instructions[doubled.a];
    const ProgramInstruction& two = program.instructions[doubled.b];
    const bool normalFirst = normal.op == ProgramOp::Geometry;
    const ProgramInstruction& geometry = normalFirst ? normal : two;
    const ProgramInstruction& scale = normalFirst ? two : normal;
    CHECK(geometry.op == ProgramOp::Geometry);
    CHECK(geometry.index == static_cast<uint16_t>(GeometryAttribute::Normal));
    CHECK(geometry.width == 3);
    CHECK(scale.op == ProgramOp::Constant);
    CHECK(program.constants[scale.index] == 2.0f);

    const ProgramInstruction& alpha = program.instructions[program.outputs[alphaOutput]];
    CHECK(alpha.op == ProgramOp::Constant);
    CHECK(program.constants[alpha.index] == 1.0f);
}

TEST(spirvEvaluatorReportsAnInvalidModule) {
    const std::array<uint32_t, 2> notSpirv{};
    ExpressionGraph graph;
    NormalInputs inputs(graph);
    evaluateSpirv(notSpirv, graph, inputs);
    CHECK(graph.getFailure() == "The shader is not a SPIR-V module.");
}
