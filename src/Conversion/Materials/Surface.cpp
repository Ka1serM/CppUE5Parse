#include "Conversion/Materials/Surface.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ue5parse::conversion {
namespace {

SurfaceAttributes gbufferAttributes(MaterialEvaluator& evaluator, const CompiledMaterialShader& basePass, const BlendMode blendMode) {
    ExpressionGraph& graph = evaluator.getGraph();
    const ShaderOutputs outputs = evaluator.evaluate(basePass, viewDefaults());
    if (outputs.targets.size() < 4) {
        graph.fail("The base pass writes no GBuffer.");
        return {};
    }
    const auto& color = outputs.targets[0];
    const auto& gbufferA = outputs.targets[1];
    const auto& gbufferB = outputs.targets[2];
    const auto& gbufferC = outputs.targets[3];

    SurfaceAttributes surface;
    surface.baseColor = {gbufferC[0], gbufferC[1], gbufferC[2]};
    for (uint32_t i = 0; i < 3; ++i)
        surface.normal[i] = graph.sub(graph.mul(gbufferA[i], graph.constant(2.0f)), graph.constant(1.0f));
    surface.metallic = gbufferB[0];
    surface.specular = gbufferB[1];
    surface.roughness = gbufferB[2];
    surface.emission = {color[0], color[1], color[2]};
    if (blendMode == BlendMode::Masked)
        surface.opacity = graph.select(outputs.discard, graph.constant(0.0f), graph.constant(1.0f));
    return surface;
}

EngineValues translucentView(const bool lit) {
    const float ambient = lit ? 1.0f / (0.282095f * 3.14159265f) : 0.0f;
    EngineValues values = viewDefaults();
    for (const char* forward : {"Forward", "ForwardISR"})
        for (const char* count : {"NumLocalLights", "NumReflectionCaptures", "HasDirectionalLight", "NumDirectionalLightCascades"})
            values[std::string("TranslucentBasePass_Shared_") + forward + "_" + count] = {0.0f};
    for (const char* volume : {"Inner", "Outer"}) {
        values[std::string("TranslucentBasePass_TranslucencyLightingVolumeAmbient") + volume] = {ambient, ambient, ambient, 1.0f};
        values[std::string("TranslucentBasePass_TranslucencyLightingVolumeDirectional") + volume] = {0.0f, 0.0f, 0.0f, 0.0f};
    }
    return values;
}

SurfaceAttributes translucentAttributes(MaterialEvaluator& evaluator, const CompiledMaterialShader& basePass) {
    ExpressionGraph& graph = evaluator.getGraph();
    const ShaderOutputs dark = evaluator.evaluate(basePass, translucentView(false));
    const ShaderOutputs lit = evaluator.evaluate(basePass, translucentView(true));
    if (dark.targets.empty() || lit.targets.empty()) {
        graph.fail("The base pass writes no render target.");
        return {};
    }
    const auto& unlitColor = dark.targets[0];
    const auto& litColor = lit.targets[0];

    SurfaceAttributes surface;
    for (uint32_t c = 0; c < 3; ++c) {
        surface.baseColor[c] = graph.sub(litColor[c], unlitColor[c]);
        surface.emission[c] = unlitColor[c];
        surface.normal[c] = graph.geometry(GeometryAttribute::Normal, c);
    }
    surface.metallic = graph.constant(0.0f);
    surface.specular = graph.constant(0.0f);
    surface.roughness = graph.constant(1.0f);
    surface.opacity = graph.clamp(unlitColor[3], graph.constant(0.0f), graph.constant(1.0f));
    return surface;
}

void requireResolved(ExpressionGraph& graph, const ExprId value, const char* attribute) {
    const auto unresolved = graph.unresolvedInputs(value);
    if (unresolved.empty())
        return;
    std::string names;
    for (const ExprId id : unresolved)
        names += (names.empty() ? "" : ", ") + graph.unresolvedName(id);
    graph.fail(std::string("The ") + attribute + " depends on values the translation cannot express: " + names + ".");
}

std::vector<std::string> unresolvedNames(const ExpressionGraph& graph, const std::span<const ExprId> outputs) {
    std::set<std::string> names;
    for (const ExprId output : outputs)
        for (const ExprId input : graph.unresolvedInputs(output))
            names.insert(graph.unresolvedName(input));
    return {names.begin(), names.end()};
}

std::vector<std::string> dropUnresolvedDisplacement(MaterialEvaluator& evaluator, SurfaceAttributes& surface) {
    std::vector<std::string> names = unresolvedNames(evaluator.getGraph(), *surface.displacement);
    if (names.empty())
        return names;
    std::string list;
    for (const std::string& name : names)
        list += (list.empty() ? "" : ", ") + name;
    evaluator.warn("The vertex shader moves the vertices (world position offset), but the offset depends on values the translation cannot express: " + list
                   + ".");
    surface.displacement.reset();
    return names;
}

class NanFreeCondition {
public:
    explicit NanFreeCondition(ExpressionGraph& graph) : graph(graph) {}

    ExprId of(const ExprId value) {
        if (const auto found = known.find(value); found != known.end())
            return found->second;
        const ExprId condition = derive(value);
        known.emplace(value, condition);
        return condition;
    }

private:
    ExprId derive(const ExprId value) {
        const ExprNode node = graph.node(value);
        switch (node.op) {
        case ExprOp::Constant:
            return graph.boolean(!std::isnan(node.value));
        case ExprOp::Texture:
        case ExprOp::Less:
        case ExprOp::LessEqual:
        case ExprOp::Equal:
        case ExprOp::NotEqual:
        case ExprOp::And:
        case ExprOp::Or:
        case ExprOp::Not:
            return graph.boolean(true);
        case ExprOp::Min:
        case ExprOp::Max:
            return graph.logicalOr(of(node.a), of(node.b));
        case ExprOp::Select:
            return graph.logicalAnd(of(node.a), graph.select(node.a, of(node.b), of(node.c)));
        case ExprOp::Sqrt:
        case ExprOp::Log2:
            return graph.logicalAnd(of(node.a), atLeast(node.a, 0.0f));
        case ExprOp::Pow:
            return graph.logicalAnd(graph.logicalAnd(of(node.a), of(node.b)), atLeast(node.a, 0.0f));
        case ExprOp::Asin:
        case ExprOp::Acos:
            return graph.logicalAnd(of(node.a), graph.logicalAnd(atLeast(node.a, -1.0f), graph.binary(ExprOp::LessEqual, node.a, graph.constant(1.0f))));
        default:
            break;
        }
        const std::array<ExprId, 3> operands{node.a, node.b, node.c};
        ExprId condition = graph.boolean(true);
        for (uint32_t i = 0; i < graph.operandCount(value); ++i)
            condition = graph.logicalAnd(condition, of(operands[i]));
        return condition;
    }

    ExprId atLeast(const ExprId value, const float low) {
        if (low == 0.0f && isNonNegative(value))
            return graph.boolean(true);
        return graph.binary(ExprOp::LessEqual, graph.constant(low), value);
    }

    bool isNonNegative(const ExprId value) const {
        const ExprNode& node = graph.node(value);
        switch (node.op) {
        case ExprOp::Constant:
            return node.value >= 0.0f;
        case ExprOp::Abs:
        case ExprOp::Sqrt:
        case ExprOp::Exp2:
        case ExprOp::Fract:
        case ExprOp::Less:
        case ExprOp::LessEqual:
        case ExprOp::Equal:
        case ExprOp::NotEqual:
        case ExprOp::And:
        case ExprOp::Or:
        case ExprOp::Not:
            return true;
        case ExprOp::Mul:
            return node.a == node.b || (isNonNegative(node.a) && isNonNegative(node.b));
        case ExprOp::Add:
        case ExprOp::Min:
            return isNonNegative(node.a) && isNonNegative(node.b);
        case ExprOp::Max:
            return isNonNegative(node.a) || isNonNegative(node.b);
        case ExprOp::Select:
            return isNonNegative(node.b) && isNonNegative(node.c);
        default:
            return false;
        }
    }

    ExpressionGraph& graph;
    std::unordered_map<ExprId, ExprId> known;
};

//! A GPU drops every triangle with a NaN vertex, so a world position offset that turns NaN culls the geometry.
void cullNanDisplacement(ExpressionGraph& graph, SurfaceAttributes& surface) {
    NanFreeCondition nanFree(graph);
    ExprId kept = graph.boolean(true);
    for (const ExprId offset : *surface.displacement)
        kept = graph.logicalAnd(kept, nanFree.of(offset));
    if (kept == graph.boolean(true))
        return;
    surface.opacity = graph.select(kept, surface.opacity.value_or(graph.constant(1.0f)), graph.constant(0.0f));
}

SurfaceAttributes shaderAttributes(MaterialEvaluator& evaluator, const std::vector<CompiledMaterialShader>& shaders, const CompiledMaterialShader& vertexShader,
                                   const BlendMode blendMode, const SurfaceOverride& surfaceOverride) {
    const CompiledMaterialShader& basePass = shaders.front();
    if (surfaceOverride)
        if (std::optional<SurfaceAttributes> surface = surfaceOverride(evaluator, shaders, blendMode))
            return *surface;
    evaluator.evaluate(vertexShader, viewDefaults(), {}, VertexProjection::Camera);
    if (blendMode >= BlendMode::Translucent)
        return translucentAttributes(evaluator, basePass);
    return gbufferAttributes(evaluator, basePass, blendMode);
}

SurfaceAttributes evaluateSurface(MaterialEvaluator& evaluator, const std::vector<CompiledMaterialShader>& shaders, const BlendMode blendMode,
                                  const SurfaceOverride& surfaceOverride) {
    ExpressionGraph& graph = evaluator.getGraph();
    const auto vertexShader = std::ranges::find(shaders, ShaderStage::Vertex, &CompiledMaterialShader::stage);
    if (vertexShader == shaders.end()) {
        graph.fail("The material has no vertex shader.");
        return {};
    }
    const ShaderOutputs vertex = evaluator.evaluate(*vertexShader, viewDefaults());
    std::array<ExprId, 3> displacement{};
    for (uint32_t c = 0; c < 3; ++c)
        displacement[c] = graph.sub(vertex.position[c], graph.geometry(GeometryAttribute::LocalPosition, c));

    SurfaceAttributes surface = shaderAttributes(evaluator, shaders, *vertexShader, blendMode, surfaceOverride);
    if (!std::ranges::all_of(displacement, [&](const ExprId offset) { return graph.isConstant(offset, 0.0f); }))
        surface.displacement = displacement;
    return surface;
}

}

SurfaceReading readSurface(MaterialEvaluator& evaluator, const std::vector<CompiledMaterialShader>& shaders, const BlendMode blendMode,
                           const SurfaceOverride& surfaceOverride) {
    ExpressionGraph& graph = evaluator.getGraph();
    SurfaceReading reading{.surface = evaluateSurface(evaluator, shaders, blendMode, surfaceOverride)};
    SurfaceAttributes& surface = reading.surface;
    if (graph.isFailed())
        return reading;
    if (surface.displacement)
        reading.unresolvedDisplacement = dropUnresolvedDisplacement(evaluator, surface);
    if (surface.displacement)
        cullNanDisplacement(graph, surface);
    for (uint32_t c = 0; c < 3; ++c) {
        requireResolved(graph, surface.baseColor[c], "base color");
        requireResolved(graph, surface.normal[c], "normal");
        requireResolved(graph, surface.emission[c], "emission");
    }
    requireResolved(graph, surface.roughness, "roughness");
    requireResolved(graph, surface.metallic, "metallic");
    requireResolved(graph, surface.specular, "specular");
    if (surface.opacity)
        requireResolved(graph, *surface.opacity, "opacity");
    if (surface.refraction)
        for (const ExprId channel : *surface.refraction)
            requireResolved(graph, channel, "refraction");
    return reading;
}

std::vector<std::string> absentValuesInSurface(const MaterialData& data, const std::vector<CompiledMaterialShader>& shaders,
                                               const SurfaceOverride& surfaceOverride) {
    MaterialEvaluator evaluator(data, AbsentValues::Traced);
    const ExpressionGraph& graph = evaluator.getGraph();
    const SurfaceAttributes surface = evaluateSurface(evaluator, shaders, data.blendMode, surfaceOverride);
    if (const std::optional<std::string>& failure = graph.getFailure())
        return {"an absent engine value changes how the shader runs: " + *failure};
    std::vector<ExprId> outputs{surface.roughness, surface.metallic, surface.specular};
    outputs.insert(outputs.end(), surface.baseColor.begin(), surface.baseColor.end());
    outputs.insert(outputs.end(), surface.normal.begin(), surface.normal.end());
    outputs.insert(outputs.end(), surface.emission.begin(), surface.emission.end());
    if (surface.opacity)
        outputs.push_back(*surface.opacity);
    if (surface.refraction)
        outputs.insert(outputs.end(), surface.refraction->begin(), surface.refraction->end());
    if (surface.displacement)
        outputs.insert(outputs.end(), surface.displacement->begin(), surface.displacement->end());
    return unresolvedNames(graph, outputs);
}

}
