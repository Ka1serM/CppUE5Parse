#include "Conversion/Materials/Surface.h"

#include <algorithm>
#include <string>
#include <string_view>

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

SurfaceAttributes readAttributes(MaterialEvaluator& evaluator, const std::vector<CompiledMaterialShader>& shaders, const BlendMode blendMode,
                                 const SurfaceOverride& surfaceOverride) {
    if (shaders.empty()) {
        evaluator.getGraph().fail("The material has no shader.");
        return {};
    }
    const CompiledMaterialShader& basePass = shaders.front();
    if (surfaceOverride)
        if (std::optional<SurfaceAttributes> surface = surfaceOverride(evaluator, shaders, blendMode))
            return *surface;
    if (blendMode >= BlendMode::Translucent)
        return translucentAttributes(evaluator, basePass);
    return gbufferAttributes(evaluator, basePass, blendMode);
}

}

SurfaceAttributes readSurface(MaterialEvaluator& evaluator, const std::vector<CompiledMaterialShader>& shaders, const BlendMode blendMode,
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
    evaluator.evaluate(*vertexShader, viewDefaults(), {}, VertexProjection::Camera);

    SurfaceAttributes surface = readAttributes(evaluator, shaders, blendMode, surfaceOverride);
    if (graph.isFailed())
        return surface;
    const bool moves = !std::ranges::all_of(displacement, [&](const ExprId offset) { return graph.isConstant(offset, 0.0f); });
    const bool resolved = std::ranges::all_of(displacement, [&](const ExprId offset) { return graph.unresolvedInputs(offset).empty(); });
    if (moves && resolved)
        surface.displacement = displacement;
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
        requireResolved(graph, *surface.refraction, "refraction");
    return surface;
}

}
