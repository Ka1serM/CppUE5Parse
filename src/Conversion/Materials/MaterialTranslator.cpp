#include "Conversion/Materials/MaterialTranslator.h"

#include <algorithm>
#include <iterator>
#include <span>
#include <utility>

#include "Conversion/Materials/MaterialEvaluator.h"
#include "Conversion/Materials/Surface.h"
#include "Conversion/Spirv/ProgramWriter.h"

namespace ue5parse::conversion {
namespace {

TextureAddress addressMode(const int address) {
    switch (address) {
    case 1:
        return TextureAddress::Clamp;
    case 2:
        return TextureAddress::Mirror;
    default:
        return TextureAddress::Wrap;
    }
}

MaterialProgram::Parameter programParameter(const NumericParameter& parameter) {
    return {std::string(parameter.name),
            parameter.vector ? std::vector<float>(parameter.value.begin(), parameter.value.end()) : std::vector<float>{parameter.value[0]}};
}

ConversionResult<MaterialProgram> writeProgram(const ExpressionGraph& graph, const SurfaceAttributes& surface, const MaterialData& data,
                                               const std::vector<NumericParameter>& collectionParameters) {
    MaterialProgram program;
    for (const NumericParameter& parameter : data.parameters)
        program.parameters.push_back(programParameter(parameter));
    if (data.vertexFactory != VertexFactory::Mesh)
        for (const NumericParameter& parameter : particleParameters)
            program.parameters.push_back(programParameter(parameter));
    for (const NumericParameter& parameter : collectionParameters)
        program.parameters.push_back(programParameter(parameter));
    for (const TextureSlot& slot : data.textures)
        if (programSamples(slot.type))
            program.textures.push_back(
                {std::string(slot.texture), std::string(slot.name), addressMode(slot.addressU), addressMode(slot.addressV), slot.nearest});

    std::vector<uint32_t> parameterWidths;
    for (const MaterialProgram::Parameter& parameter : program.parameters)
        parameterWidths.push_back(static_cast<uint32_t>(parameter.value.size()));

    ProgramWriter writer(graph, parameterWidths);
    const uint32_t baseColor = writer.write(std::span<const ExprId, 3>(surface.baseColor));
    const uint32_t normal = writer.write(std::span<const ExprId, 3>(surface.normal));
    const uint32_t emission = writer.write(std::span<const ExprId, 3>(surface.emission));
    const uint32_t roughness = writer.write(surface.roughness);
    const uint32_t metallic = writer.write(surface.metallic);
    const uint32_t specular = writer.write(surface.specular);
    const std::optional<uint32_t> opacity = surface.opacity ? std::optional(writer.write(*surface.opacity)) : std::nullopt;
    const std::optional<uint32_t> refraction = surface.refraction ? std::optional(writer.write(std::span<const ExprId, 3>(*surface.refraction))) : std::nullopt;
    const std::optional<uint32_t> displacement =
        surface.displacement ? std::optional(writer.write(std::span<const ExprId, 3>(*surface.displacement))) : std::nullopt;
    if (writer.getFailure())
        return conversionFailure(*writer.getFailure());

    ExpressionProgram written = writer.finish();
    const auto outputOf = [&](const std::optional<uint32_t> slot) { return slot ? static_cast<int32_t>(written.outputs[*slot]) : -1; };
    program.baseColor = written.outputs[baseColor];
    program.normal = written.outputs[normal];
    program.emission = written.outputs[emission];
    program.roughness = written.outputs[roughness];
    program.metallic = written.outputs[metallic];
    program.specular = written.outputs[specular];
    program.opacity = outputOf(opacity);
    program.refraction = outputOf(refraction);
    program.displacement = outputOf(displacement);
    program.instructions = std::move(written.instructions);
    program.constants = std::move(written.constants);
    return program;
}

}

ConversionResult<MaterialTranslation> translateMaterial(const MaterialData& data, const std::vector<CompiledMaterialShader>& shaders,
                                                        const SurfaceOverride& surfaceOverride) {
    MaterialEvaluator evaluator(data);
    SurfaceReading reading = readSurface(evaluator, shaders, data.blendMode, surfaceOverride);
    if (const std::optional<std::string>& failure = evaluator.getGraph().getFailure())
        return conversionFailure(*failure);
    ConversionResult<MaterialProgram> program = writeProgram(evaluator.getGraph(), reading.surface, data, evaluator.getCollectionParameters());
    if (!program)
        return std::unexpected(std::move(program.error()));
    std::vector<std::string> warnings = evaluator.getWarnings().getMessages();
    std::ranges::move(evaluator.readsAbsentValue() ? absentValuesInSurface(data, shaders, surfaceOverride) : std::move(reading.unresolvedDisplacement),
                      std::back_inserter(warnings));
    return MaterialTranslation{std::move(*program), std::move(warnings)};
}

}
