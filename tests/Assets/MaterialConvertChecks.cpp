#include <algorithm>
#include <array>
#include <deque>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "AssetSuite.h"
#include "Conversion/Materials/MaterialShaderCompiler.h"
#include "Conversion/Materials/MaterialShaderExport.h"
#include "Conversion/Materials/MaterialTranslator.h"
#include "Conversion/Materials/MaterialWgsl.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialParameterCollection.h"
#include "Engine/Source/Runtime/RenderCore/ShaderCodeArchive.h"

namespace test::assets {

using namespace ue5parse;
namespace cv = ue5parse::conversion;

namespace {

constexpr std::array<std::string_view, 2> platforms = {"PCD3D_SM6", "PCD3D_SM5"};
constexpr std::array<std::string_view, 3> meshVertexFactories = {"FLocalVertexFactory", "TGPUSkinVertexFactoryDefault", "FInstancedStaticMeshVertexFactory"};
constexpr std::array<rt::EMaterialQualityLevel, 5> qualities = {rt::EMaterialQualityLevel::Num, rt::EMaterialQualityLevel::Epic, rt::EMaterialQualityLevel::High,
                                                                rt::EMaterialQualityLevel::Medium, rt::EMaterialQualityLevel::Low};

template <class Range, class T>
std::size_t rank(const Range& range, const T& value) {
    return static_cast<std::size_t>(std::ranges::find(range, value) - std::ranges::begin(range));
}

//! The base pass pixel shader without a light map policy is the one the material converter reads the surface from.
int shaderPreference(std::string_view name) {
    if (name == "TBasePassPSFNoLightMapPolicy")
        return 0;
    if (name.starts_with("TBasePassPSF"))
        return 1;
    if (!name.contains("BasePassPSF"))
        return 4;
    return name.contains("FallbackPolicy") || name.contains("NoLightMapPolicy") ? 2 : 3;
}

const cv::MaterialShaderInfo* findPixelShader(const std::vector<cv::MaterialShaderInfo>& shaders) {
    const cv::MaterialShaderInfo* best = nullptr;
    std::tuple<std::size_t, int, std::size_t, std::size_t> bestOrder;
    for (const cv::MaterialShaderInfo& info : shaders) {
        const std::string platform = info.shaderMap->ShaderPlatformName.toString();
        if (!std::ranges::contains(platforms, platform) || info.shader->GetFrequency() != rt::SF_Pixel)
            continue;
        const std::size_t factory =
            rank(meshVertexFactories | std::views::transform([](std::string_view name) { return rt::FHashedName(name); }), info.meshShaderMap->VertexFactoryTypeName);
        const std::tuple order{factory, shaderPreference(info.typeName), rank(platforms, platform), rank(qualities, info.shaderMap->ShaderMapId.QualityLevel)};
        if (best == nullptr || order < bestOrder) {
            best = &info;
            bestOrder = order;
        }
    }
    return best;
}

const cv::MaterialShaderInfo* findVertexShader(const std::vector<cv::MaterialShaderInfo>& shaders, const cv::MaterialShaderInfo& pixel) {
    std::string vertexName = pixel.typeName;
    if (const std::size_t at = vertexName.find("PSF"); at != std::string::npos)
        vertexName.replace(at, 3, "VSF");
    const auto found = std::ranges::find_if(shaders, [&](const cv::MaterialShaderInfo& info) {
        return info.meshShaderMap == pixel.meshShaderMap && info.shader->GetFrequency() == rt::SF_Vertex && !info.typeName.empty() && vertexName.starts_with(info.typeName);
    });
    return found != shaders.end() ? &*found : nullptr;
}

cv::BlendMode blendMode(rt::EBlendMode mode) {
    if (mode > rt::BLEND_AlphaHoldout)
        return cv::BlendMode::Translucent;
    return static_cast<cv::BlendMode>(mode);
}

cv::ParameterCollection parameterCollection(const rt::UMaterialParameterCollection* collection, std::deque<std::string>& strings) {
    cv::ParameterCollection layout;
    if (collection == nullptr)
        return layout;
    const std::string name = collection->GetName().ToUtf8();
    const auto keep = [&](const rt::FName& parameter) -> std::string_view { return strings.emplace_back(cv::collectionParameterName(name, parameter.toString())); };
    for (const rt::FCollectionScalarParameter& parameter : collection->ScalarParameters)
        layout.scalars.push_back({keep(parameter.ParameterName), false, {parameter.DefaultValue}});
    for (const rt::FCollectionVectorParameter& parameter : collection->VectorParameters) {
        const rt::FLinearColor& value = parameter.DefaultValue;
        layout.vectors.push_back({keep(parameter.ParameterName), true, {value.R, value.G, value.B, value.A}});
    }
    return layout;
}

bool failConversion(const std::string& path, const cv::ConversionError& error) {
    if (error.cause)
        printError(path, *error.cause);
    return fail(path, error.message);
}

//! Converts the material the way UnrealKit does: the base pass pixel and vertex shaders to validated SPIR-V, then the surface to a material program and WGSL.
void convertMaterial(const Cook& cook, const rt::FShaderCodeLibrary& library, cv::SpirvCache& cache, const std::string& path) {
    rt::UMaterialInterface* material = loadObject<rt::UMaterialInterface>(path);
    if (material == nullptr)
        return;
    Result<std::optional<cv::MaterialShaderMaps>> maps = cv::loadMaterialShaderMaps(*material);
    if (!maps)
        printError(path, maps.error());
    //! The cook writes no shader map for a material whose shaders did not compile, as the shaders check accepts.
    if (!CHECK(maps.has_value()) || !maps->has_value())
        return;
    const std::vector<cv::MaterialShaderInfo> shaders = cv::getMaterialShaders(**maps, library);
    const cv::MaterialShaderInfo* pixel = findPixelShader(shaders);
    if (pixel == nullptr) {
        fail(path, "has no PC D3D pixel shader");
        return;
    }
    const cv::MaterialShaderInfo* vertex = findVertexShader(shaders, *pixel);
    if (vertex == nullptr) {
        fail(path, std::format("has no vertex shader for {}", pixel->typeName));
        return;
    }
    const std::array<const cv::MaterialShaderInfo*, 2> selected{pixel, vertex};
    Result<cv::MaterialShaderExport> exported = cv::exportMaterialShaders(*material, **maps, selected, library);
    if (!exported)
        printError(path, exported.error());
    if (!CHECK(exported.has_value()))
        return;

    std::deque<std::string> strings;
    const auto keep = [&](std::string text) -> std::string_view { return strings.emplace_back(std::move(text)); };
    static constexpr std::array stages{cv::ShaderStage::Pixel, cv::ShaderStage::Vertex};
    std::vector<cv::ShaderData> shaderData;
    for (std::size_t index = 0; index < exported->shaders.size(); ++index) {
        const cv::MaterialShaderCode& code = exported->shaders[index];
        shaderData.push_back({code.typeName, stages[index], {reinterpret_cast<const uint8_t*>(code.code.data()), code.code.size()}, code.hash, code.uniformBufferNames});
    }
    const rt::FUniformExpressionSet& expressions = exported->shaderMap->Content.MaterialCompilationOutput.UniformExpressionSet;
    std::vector<cv::NumericParameter> parameters;
    for (const cv::MaterialNumericParameterValue& parameter : exported->numericParameters)
        parameters.push_back({keep(parameter.name), parameter.type == rt::EMaterialParameterType::Vector, {parameter.value.R, parameter.value.G, parameter.value.B, parameter.value.A}});
    std::vector<cv::PreshaderProgram> preshaders;
    for (const rt::FMaterialUniformPreshaderHeader& header : expressions.UniformPreshaders)
        preshaders.push_back({header.OpcodeOffset, header.OpcodeSize, header.FieldIndex, header.NumFields});
    std::vector<cv::PreshaderField> fields;
    for (const rt::FMaterialUniformPreshaderField& field : expressions.UniformPreshaderFields)
        fields.push_back({field.BufferOffset, field.ComponentIndex, field.Type});
    std::vector<std::string_view> preshaderNames;
    for (const rt::FName& name : expressions.UniformPreshaderData.Names)
        preshaderNames.push_back(keep(name.toString()));
    std::vector<cv::TextureSlot> textures;
    for (const cv::MaterialTextureSlot& slot : exported->textureSlots)
        textures.push_back({keep(slot.parameterName), slot.texture != nullptr ? keep(getPathName(slot.texture)) : std::string_view(), slot.type, slot.index, slot.addressX,
                            slot.addressY, slot.nearest, slot.size});
    std::vector<cv::TextureExtent> textureSizes;
    for (const cv::MaterialTextureExtent& extent : exported->textureExtents)
        textureSizes.push_back({keep(extent.parameterName), extent.referencedTextureIndex, extent.sizeX, extent.sizeY, extent.numSlices});
    std::vector<cv::ParameterCollection> collections;
    for (const rt::UMaterialParameterCollection* collection : exported->parameterCollections)
        collections.push_back(parameterCollection(collection, strings));
    const cv::MaterialData data{
        .owner = path,
        .blendMode = blendMode(material->GetBlendMode().GetValue()),
        .game = cook.game,
        .shaders = shaderData,
        .parameters = parameters,
        .preshaders = preshaders,
        .fields = fields,
        .preshaderCode = {expressions.UniformPreshaderData.Data.GetData(), static_cast<std::size_t>(expressions.UniformPreshaderData.Data.Num())},
        .preshaderNames = preshaderNames,
        .uniformExpressions = &expressions,
        .textures = textures,
        .textureSizes = textureSizes,
        .parameterCollections = collections,
    };

    std::vector<cv::CompiledMaterialShader> compiled;
    for (const cv::ShaderData& shader : shaderData) {
        cv::ConversionResult<cv::CompiledMaterialShader> spirv = cv::compileMaterialShader(data, shader, cache);
        if (!spirv) {
            failConversion(describe(path, shader.type), spirv.error());
            return;
        }
        compiled.push_back(std::move(*spirv));
    }
    cv::ConversionResult<cv::MaterialTranslation> translated = cv::translateMaterial(data, compiled);
    if (!translated) {
        failConversion(path, translated.error());
        return;
    }
    CHECK(!cv::writeMaterialWgsl(translated->program).code.empty());
}

void checkConversion(const Cook& cook) {
    std::vector<std::string> materials = findMaterials(cook);
    if (materials.empty()) {
        fail(cook.version, "the reference values list no materials");
        return;
    }
    withRuntime(cook, [&](const FileProvider& provider) {
        rt::FShaderCodeLibrary library(provider);
        Result<void> opened = library.OpenLibraries();
        if (!opened)
            printError("shader library", opened.error());
        if (!CHECK(opened.has_value()))
            return;
        cv::SpirvCache cache;
        for (const std::string& path : materials)
            convertMaterial(cook, library, cache, path);
    });
}

}

void addMaterialConvertChecks(Checks& checks) {
    checks.cook.emplace("material.convert", checkConversion);
}

}
