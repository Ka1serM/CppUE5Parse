#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <deque>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "AssetSuite.h"
#include "Conversion/Materials/MaterialShaderCompiler.h"
#include "Conversion/Materials/MaterialShaderExport.h"
#include "Conversion/Materials/MaterialTranslator.h"
#include "Conversion/Materials/MaterialWgsl.h"
#include "Conversion/Materials/Preshader.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/Materials/Material.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialParameterCollection.h"
#include "Engine/Source/Runtime/RenderCore/ShaderCodeArchive.h"

namespace test::assets {

using namespace ue5parse;
namespace cv = ue5parse::conversion;

namespace {

constexpr std::array<std::string_view, 2> platforms = {"PCD3D_SM6", "PCD3D_SM5"};
constexpr std::array<std::string_view, 3> meshVertexFactories = {"FLocalVertexFactory", "TGPUSkinVertexFactoryDefault", "FInstancedStaticMeshVertexFactory"};
constexpr std::array<rt::EMaterialQualityLevel, 5> qualities = {rt::EMaterialQualityLevel::Num, rt::EMaterialQualityLevel::Epic,
                                                                rt::EMaterialQualityLevel::High, rt::EMaterialQualityLevel::Medium,
                                                                rt::EMaterialQualityLevel::Low};

struct UncompiledMaterial {
    std::string_view path;
    EGame first;
    EGame last;
};

//! The RefGen materials whose shaders the engine rejects, with the error from the engine source; the cook writes them with bValid false.
constexpr UncompiledMaterial uncompiledMaterials[] = {
    //! HLSLMaterialTranslator: "Translucency after DOF with BLEND_Modulate is not supported" until UE 4.25 allows it with dual source blending.
    {"/Game/Catalog/Materials/M_Blend_Modulate.M_Blend_Modulate", EGame::GAME_UE4_0, EGame::GAME_UE4_24},
    //! HLSLMaterialTranslator: UE 5.5 requires "BeforeDOF" for Alpha Holdout, and new materials translate after DOF.
    {"/Game/Catalog/Materials/M_Blend_AlphaHoldout.M_Blend_AlphaHoldout", EGame::GAME_UE5_5, EGame::GAME_UE5_LATEST},
    //! The factories create ML_Base, ML_Top and MLB_Ref without an output, so the layer stack has "Invalid function output".
    {"/Game/Catalog/Materials/M_Layered.M_Layered", EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST},
    {"/Game/Catalog/Materials/MI_Layered.MI_Layered", EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST},
    //! The sample keeps its default BaseColor material type, and RVT_Ref is BaseColor_Normal_Specular: "Material type is 'BaseColor', should be ...".
    {"/Game/Catalog/Materials/M_RVT_Sample.M_RVT_Sample", EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST},
    //! "SingleLayerWater materials requires the use of SingleLayerWaterMaterial output node."
    {"/Game/Catalog/Materials/M_SM_SingleLayerWater.M_SM_SingleLayerWater", EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST},
    //! The texture array sample has no coordinates: "UVW input required for texturearray sample".
    {"/Game/Catalog/Materials/M_TextureArray.M_TextureArray", EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST},
};

bool isUncompiled(const Cook& cook, std::string_view path) {
    return std::ranges::any_of(uncompiledMaterials, [&](const UncompiledMaterial& material) {
        return material.path == path && cook.game >= material.first && cook.game <= material.last;
    });
}

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
        const std::size_t factory = rank(meshVertexFactories | std::views::transform([](std::string_view name) { return rt::FHashedName(name); }),
                                         info.meshShaderMap->VertexFactoryTypeName);
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
        return info.meshShaderMap == pixel.meshShaderMap && info.shader->GetFrequency() == rt::SF_Vertex && !info.typeName.empty()
               && vertexName.starts_with(info.typeName);
    });
    return found != shaders.end() ? &*found : nullptr;
}

cv::BlendMode blendMode(rt::EBlendMode mode) {
    if (mode > rt::BLEND_AlphaHoldout)
        return cv::BlendMode::Translucent;
    return static_cast<cv::BlendMode>(mode);
}

//! The names of EBlendMode in the reference values, which the Python API of the engine writes.
constexpr std::pair<std::string_view, cv::BlendMode> blendModeNames[] = {
    {"BLEND_OPAQUE", cv::BlendMode::Opaque},
    {"BLEND_MASKED", cv::BlendMode::Masked},
    {"BLEND_TRANSLUCENT", cv::BlendMode::Translucent},
    {"BLEND_ADDITIVE", cv::BlendMode::Additive},
    {"BLEND_MODULATE", cv::BlendMode::Modulate},
    {"BLEND_ALPHA_COMPOSITE", cv::BlendMode::AlphaComposite},
    {"BLEND_ALPHA_HOLDOUT", cv::BlendMode::AlphaHoldout},
    {"BLEND_TRANSLUCENT_COLORED_TRANSMITTANCE", cv::BlendMode::Translucent},
};

cv::ParameterCollection parameterCollection(const rt::UMaterialParameterCollection* collection, std::deque<std::string>& strings) {
    cv::ParameterCollection layout;
    if (collection == nullptr)
        return layout;
    const std::string name = collection->GetName().ToUtf8();
    const auto keep = [&](const rt::FName& parameter) -> std::string_view {
        return strings.emplace_back(cv::collectionParameterName(name, parameter.toString()));
    };
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

using Lanes = std::array<float, 4>;

//! Runs a material program on the CPU the way MaterialWgsl writes it, for the instructions that read no texture, geometry or time.
class ProgramValues {
public:
    explicit ProgramValues(const cv::MaterialProgram& program) : program(program) {
        values.reserve(program.instructions.size());
        for (const cv::ProgramInstruction& instruction : program.instructions)
            values.push_back(evaluate(instruction));
    }

    std::optional<Lanes> get(uint32_t id) const { return id < values.size() ? values[id] : std::nullopt; }

private:
    static Lanes splat(float value) { return {value, value, value, value}; }

    uint32_t width(uint32_t id) const { return id < program.instructions.size() ? program.instructions[id].width : 1u; }

    Lanes masked(const Lanes& value, uint32_t count) const {
        Lanes result{};
        for (uint32_t lane = 0; lane < std::min(std::max(count, 1u), 4u); ++lane)
            result[lane] = value[lane];
        return result;
    }

    static float dot(const Lanes& a, const Lanes& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]; }

    template <class Function>
    static Lanes map(const Lanes& a, const Lanes& b, const Lanes& c, Function function) {
        Lanes result{};
        for (std::size_t lane = 0; lane < 4; ++lane)
            result[lane] = function(a[lane], b[lane], c[lane]);
        return result;
    }

    std::optional<Lanes> constant(const cv::ProgramInstruction& instruction) const {
        Lanes lanes{};
        for (uint32_t lane = 0; lane < 4; ++lane) {
            const std::size_t at = instruction.index + (instruction.width == 1 ? 0 : lane);
            if ((instruction.width == 1 || lane < instruction.width) && at < program.constants.size())
                lanes[lane] = program.constants[at];
        }
        return lanes;
    }

    std::optional<Lanes> parameter(const cv::ProgramInstruction& instruction) const {
        if (instruction.index >= program.parameters.size())
            return std::nullopt;
        const std::vector<float>& value = program.parameters[instruction.index].value;
        if (value.size() == 1)
            return splat(value[0]);
        Lanes lanes{};
        std::ranges::copy(value | std::views::take(4), lanes.begin());
        return lanes;
    }

    std::optional<Lanes> evaluate(const cv::ProgramInstruction& instruction) const {
        using enum cv::ProgramOp;
        if (instruction.op == Constant)
            return constant(instruction);
        if (instruction.op == Parameter)
            return parameter(instruction);
        const std::optional<Lanes> a = get(instruction.a), b = get(instruction.b), c = get(instruction.c);
        const auto known = [&](int count) { return a && (count < 2 || b) && (count < 3 || c); };
        const std::optional<Lanes> result = compute(instruction, a, b, c, known);
        if (!result || instruction.width != 1)
            return result;
        return splat((*result)[0]);
    }

    template <class Known>
    std::optional<Lanes> compute(const cv::ProgramInstruction& instruction, const std::optional<Lanes>& a, const std::optional<Lanes>& b,
                                 const std::optional<Lanes>& c, Known known) const {
        using enum cv::ProgramOp;
        const auto unary = [&](auto function) -> std::optional<Lanes> {
            if (!known(1))
                return std::nullopt;
            return map(*a, *a, *a, [&](float x, float, float) { return function(x); });
        };
        const auto binary = [&](auto function) -> std::optional<Lanes> {
            if (!known(2))
                return std::nullopt;
            return map(*a, *b, *b, [&](float x, float y, float) { return function(x, y); });
        };
        const auto ternary = [&](auto function) -> std::optional<Lanes> {
            if (!known(3))
                return std::nullopt;
            return map(*a, *b, *c, function);
        };
        const uint32_t operandWidth = std::max(width(instruction.a), width(instruction.b));
        switch (instruction.op) {
        case Extract:
            if (!known(1))
                return std::nullopt;
            return splat((*a)[std::min<uint32_t>(instruction.index, 3u)]);
        case Combine:
            if (!known(instruction.width >= 3 ? 3 : 2))
                return std::nullopt;
            return Lanes{(*a)[0], (*b)[0], instruction.width >= 3 ? (*c)[0] : 0.0f, 0.0f};
        case Add:
            return binary([](float x, float y) { return x + y; });
        case Sub:
            return binary([](float x, float y) { return x - y; });
        case Mul:
            return binary([](float x, float y) { return x * y; });
        case Div:
            return binary([](float x, float y) { return x / y; });
        case Min:
            return binary([](float x, float y) { return std::min(x, y); });
        case Max:
            return binary([](float x, float y) { return std::max(x, y); });
        case Pow:
            return binary([](float x, float y) { return x <= 0.0f ? 0.0f : std::pow(std::max(x, 1e-30f), y); });
        case Atan2:
            return binary([](float x, float y) { return std::atan2(x, y); });
        case Mod:
            return binary([](float x, float y) { return x - y * std::floor(x / y); });
        case Abs:
            return unary([](float x) { return std::abs(x); });
        case Floor:
            return unary([](float x) { return std::floor(x); });
        case Ceil:
            return unary([](float x) { return std::ceil(x); });
        case Fract:
            return unary([](float x) { return x - std::floor(x); });
        case Round:
            return unary([](float x) { return std::nearbyint(x); });
        case Trunc:
            return unary([](float x) { return std::trunc(x); });
        case Sign:
            return unary([](float x) { return x > 0.0f ? 1.0f : x < 0.0f ? -1.0f : 0.0f; });
        case Sqrt:
            return unary([](float x) { return std::sqrt(std::max(x, 0.0f)); });
        case Exp2:
            return unary([](float x) { return std::exp2(x); });
        case Log2:
            return unary([](float x) { return std::log2(x); });
        case Sin:
            return unary([](float x) { return std::sin(x); });
        case Cos:
            return unary([](float x) { return std::cos(x); });
        case Tan:
            return unary([](float x) { return std::tan(x); });
        case Asin:
            return unary([](float x) { return std::asin(std::clamp(x, -1.0f, 1.0f)); });
        case Acos:
            return unary([](float x) { return std::acos(std::clamp(x, -1.0f, 1.0f)); });
        case Atan:
            return unary([](float x) { return std::atan(x); });
        case Less:
            return binary([](float x, float y) { return x < y ? 1.0f : 0.0f; });
        case LessEqual:
            return binary([](float x, float y) { return x <= y ? 1.0f : 0.0f; });
        case Equal:
            return binary([](float x, float y) { return x == y ? 1.0f : 0.0f; });
        case NotEqual:
            return binary([](float x, float y) { return x != y ? 1.0f : 0.0f; });
        case Select:
            return ternary([](float condition, float whenTrue, float whenFalse) { return condition != 0.0f ? whenTrue : whenFalse; });
        case Dot:
            if (!known(2))
                return std::nullopt;
            return splat(dot(masked(*a, operandWidth), masked(*b, operandWidth)));
        case Cross:
            if (!known(2))
                return std::nullopt;
            return Lanes{(*a)[1] * (*b)[2] - (*a)[2] * (*b)[1], (*a)[2] * (*b)[0] - (*a)[0] * (*b)[2], (*a)[0] * (*b)[1] - (*a)[1] * (*b)[0], 0.0f};
        case Length:
            if (!known(1))
                return std::nullopt;
            return splat(std::sqrt(dot(masked(*a, width(instruction.a)), masked(*a, width(instruction.a)))));
        case Distance: {
            if (!known(2))
                return std::nullopt;
            const Lanes difference = map(masked(*a, operandWidth), masked(*b, operandWidth), *a, [](float x, float y, float) { return x - y; });
            return splat(std::sqrt(dot(difference, difference)));
        }
        case Normalize: {
            if (!known(1))
                return std::nullopt;
            const Lanes lanes = masked(*a, instruction.width);
            const float length = std::max(std::sqrt(dot(lanes, lanes)), 1e-30f);
            return map(lanes, lanes, lanes, [&](float x, float, float) { return x / length; });
        }
        case Clamp:
            return ternary([](float x, float low, float high) { return std::min(std::max(x, low), high); });
        case Mix:
            return ternary([](float x, float y, float t) { return x + (y - x) * t; });
        case Smoothstep:
            return ternary([](float x, float low, float high) {
                const float t = std::clamp((x - low) / (high - low), 0.0f, 1.0f);
                return t * t * (3.0f - 2.0f * t);
            });
        case MulAdd:
            return ternary([](float x, float y, float z) { return x * y + z; });
        default:
            return std::nullopt;
        }
    }

    const cv::MaterialProgram& program;
    std::vector<std::optional<Lanes>> values;
};

const Json* findValues(const Cook& cook, std::string_view kind, std::string_view path) {
    const Json* values = cook.expected.find("values");
    const Json* entries = values == nullptr ? nullptr : values->find(kind);
    return entries == nullptr ? nullptr : entries->find(path);
}

//! The engines before UE 4.23 have no Python API, so their reference values hold no material values.
bool hasMaterialValues(const Cook& cook) {
    const Json* values = cook.expected.find("values");
    return values != nullptr && values->find("material.blend") != nullptr;
}

std::vector<std::string> findConvertedMaterials(const Cook& cook) {
    std::vector<std::string> paths;
    if (const Json* assets = cook.expected.find("assets"))
        for (const auto& [path, assetClass] : assets->getObject())
            if (assetClass.getString() == "Material" || assetClass.getString() == "MaterialInstanceConstant")
                paths.push_back(path);
    return paths;
}

void checkUncompiled(const Cook& cook, const rt::UMaterialInterface& material, const std::string& path) {
    if (!isUncompiled(cook, path)) {
        fail(path, "has no shader map and is not listed as a material whose shaders did not compile");
        return;
    }
    const rt::UMaterial* root = material.GetMaterial();
    if (!CHECK(root != nullptr))
        return;
    Result<rt::TArray<rt::FMaterialResource>> resources = root->LoadMaterialResources();
    if (!resources)
        printError(path, resources.error());
    if (!resources || resources->Num() == 0)
        fail(path, "has no material resources, so the cook did not record a failed compilation");
}

void checkBlendMode(const Cook& cook, const rt::UMaterialInterface& material, cv::BlendMode actual, const std::string& path) {
    const rt::UMaterial* root = material.GetMaterial();
    if (!CHECK(root != nullptr))
        return;
    const Json* expected = findValues(cook, "material.blend", getPathName(root));
    if (expected == nullptr) {
        fail(path, "the reference values have no blend mode of its material");
        return;
    }
    const auto found = std::ranges::find(blendModeNames, expected->getString(), &std::pair<std::string_view, cv::BlendMode>::first);
    if (found == std::end(blendModeNames)) {
        fail(describe(path, "blend"), std::format("has the unknown reference blend mode {}", expected->getString()));
        return;
    }
    checkNear(static_cast<double>(actual), static_cast<double>(found->second), 0.0, describe(path, "blend"));
}

struct TranslatorParameter {
    std::string_view name;
    EGame first;
    Lanes value;
};

//! The parameters that the HLSL material translator adds with a fixed default, so the Python API of the editor does not list them.
constexpr TranslatorParameter translatorParameters[] = {
    //! Material.cpp compiles the editor selection highlight as VectorParameter(NAME_SelectionColor, FLinearColor::Black) until UE 4.24 makes it Transparent.
    {"SelectionColor", EGame::GAME_UE4_0, {0.0f, 0.0f, 0.0f, 1.0f}},
    {"SelectionColor", EGame::GAME_UE4_24, {0.0f, 0.0f, 0.0f, 0.0f}},
    //! ScalarParameter("RefractionDepthBias", GetRefractionDepthBiasValue()), and RefGen keeps the default of 0.
    {"RefractionDepthBias", EGame::GAME_UE4_0, {0.0f}},
    //! ScalarParameter("__SubsurfaceProfile", 1.0f), which the renderer replaces with the profile index.
    {"__SubsurfaceProfile", EGame::GAME_UE4_0, {1.0f}},
};

const TranslatorParameter* findTranslatorParameter(const Cook& cook, std::string_view name) {
    const TranslatorParameter* found = nullptr;
    for (const TranslatorParameter& parameter : translatorParameters)
        if (parameter.name == name && cook.game >= parameter.first)
            found = &parameter;
    return found;
}

const Json* findParameterValue(const Cook& cook, std::string_view path, const cv::MaterialNumericParameterValue& parameter) {
    const Json* values = findValues(cook, parameter.type == rt::EMaterialParameterType::Vector ? "material.vector" : "material.scalar", path);
    return values == nullptr ? nullptr : values->find(parameter.name);
}

void checkNumericParameters(const Cook& cook, const cv::MaterialShaderExport& exported, const cv::MaterialProgram& program, const std::string& path) {
    for (std::size_t index = 0; index < exported.numericParameters.size(); ++index) {
        const cv::MaterialNumericParameterValue& parameter = exported.numericParameters[index];
        const std::string what = describe(path, parameter.name);
        const std::vector<float>& translated = program.parameters[index].value;
        if (!checkText(program.parameters[index].name, parameter.name, describe(what, "program")))
            continue;
        const double exportedValues[] = {parameter.value.R, parameter.value.G, parameter.value.B, parameter.value.A};
        for (std::size_t component = 0; component < translated.size(); ++component)
            checkNear(translated[component], exportedValues[component], 0.0, describe(what, std::format("program.{}", component)));
        if (const TranslatorParameter* engine = findTranslatorParameter(cook, parameter.name)) {
            const uint32_t count = parameter.type == rt::EMaterialParameterType::Vector ? 4 : 1;
            for (uint32_t component = 0; component < count; ++component)
                checkNear(exportedValues[component], engine->value[component], valueTolerance, describe(what, std::format("{}", component)));
            continue;
        }
        if (parameter.name.empty() || !hasMaterialValues(cook))
            continue;
        const Json* expected = findParameterValue(cook, path, parameter);
        if (expected == nullptr) {
            fail(what, "the reference values have no such parameter");
            continue;
        }
        if (parameter.type == rt::EMaterialParameterType::Vector)
            checkColor(parameter.value, *expected, what);
        else
            checkNear(parameter.value.R, expected->getNumber(), valueTolerance, what);
    }
}

void checkTextureSlots(const Cook& cook, const cv::MaterialShaderExport& exported, const cv::MaterialProgram& program, const std::string& path) {
    for (const cv::MaterialTextureSlot& slot : exported.textureSlots) {
        if (slot.texture == nullptr) {
            fail(describe(path, std::format("slot {} {}", slot.index, slot.parameterName)), "has a texture slot without a texture");
            continue;
        }
        const std::string texture = getPathName(slot.texture);
        if (slot.parameterName.empty() || !hasMaterialValues(cook))
            continue;
        const Json* textures = findValues(cook, "material.texture", path);
        const Json* expected = textures == nullptr ? nullptr : textures->find(slot.parameterName);
        if (expected == nullptr)
            fail(describe(path, slot.parameterName), "the reference values have no such texture parameter");
        else
            checkText(texture, expected->getString(), describe(path, slot.parameterName));
    }
    for (const cv::MaterialProgram::Texture& texture : program.textures)
        if (!std::ranges::any_of(exported.textureSlots,
                                 [&](const cv::MaterialTextureSlot& slot) { return slot.texture != nullptr && getPathName(slot.texture) == texture.image; }))
            fail(describe(path, texture.image), "the material program samples a texture that no texture slot holds");
    const Json* textures = hasMaterialValues(cook) ? findValues(cook, "material.texture", path) : nullptr;
    if (textures == nullptr)
        return;
    for (const auto& [name, expected] : textures->getObject())
        if (!std::ranges::contains(exported.textureSlots, name, &cv::MaterialTextureSlot::parameterName))
            fail(describe(path, name), "has no texture slot for the referenced texture parameter");
}

//! Every word of the uniform buffer that the preshaders fill must evaluate to a finite constant once the parameters take their exported values.
void checkPreshaders(const cv::MaterialData& data, const std::string& path) {
    cv::ExpressionGraph graph;
    cv::Warnings warnings;
    const std::vector<cv::ExprId> words = cv::materialUniforms(graph, data, warnings);
    for (const std::string& warning : warnings.getMessages())
        fail(describe(path, "preshader"), warning);
    std::unordered_map<cv::ExprId, cv::ExprId> values;
    for (uint32_t index = 0; index < data.parameters.size(); ++index)
        for (uint32_t component = 0; component < 4; ++component)
            values.emplace(graph.parameter(index, component), graph.constant(data.parameters[index].value[component]));
    for (std::size_t word = 0; word < words.size(); ++word) {
        const cv::ExprId value = graph.substitute(words[word], values);
        const std::string what = describe(path, std::format("preshader.{}", word));
        if (!graph.isConstant(value))
            fail(what, "does not evaluate to a constant");
        else if (!std::isfinite(graph.node(value).value))
            fail(what, std::format("evaluates to {}", graph.node(value).value));
    }
}

enum class SurfaceOutput { BaseColor, Emission, Roughness, Metallic, Opacity };

struct ConstantSurface {
    std::string_view path;
    SurfaceOutput output;
    uint32_t components;
    EGame first;
    EGame last;
    Lanes value;
};

constexpr std::string_view catalog = "/Game/Catalog/Materials/";
constexpr Lanes refGenBaseColor{0.8f, 0.5f, 0.3f, 0.0f};
constexpr Lanes refGenRoughness{0.4f, 0.0f, 0.0f, 0.0f};
constexpr Lanes refGenEmissive{0.2f, 0.6f, 1.0f, 0.0f};

//! The constant surface outputs that tools/RefGen gives the materials, from RefGenMaterials.cpp and RefGenCatalog.cpp.
constexpr ConstantSurface constantSurfaces[] = {
    {"/Game/Materials/M_Opaque.M_Opaque", SurfaceOutput::Metallic, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.1f}},
    {"/Game/Materials/MI_Blue.MI_Blue", SurfaceOutput::Metallic, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.1f}},
    {"/Game/Materials/MI_Blue_Child.MI_Blue_Child", SurfaceOutput::Metallic, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.9f}},
    //! UE 5.0 and newer give M_Translucent per-pixel forward lighting, which lights it only with the scene lights.
    {"/Game/Materials/M_Translucent.M_Translucent", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_20, EGame::GAME_UE4_27, {0.2f, 0.5f, 1.0f}},
    {"M_WorldPositionOffset", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.3f, 0.9f, 0.3f}},
    //! UE 5.0 and newer use the layer preview weight only in preview compiles, so the cooked shader blends no layer.
    {"M_Landscape", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE4_27, {0.1f, 0.4f, 0.05f}},
    {"M_Landscape", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.8f}},
    {"M_Domain_Volume", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_20, EGame::GAME_UE5_LATEST, {0.7f, 0.7f, 0.8f}},
    //! An additive material writes its emissive color multiplied by its opacity, 0.5.
    {"M_Blend_Additive", SurfaceOutput::Emission, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.1f, 0.3f, 0.5f}},
    {"M_Blend_AlphaComposite", SurfaceOutput::Emission, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenEmissive},
    {"M_Blend_AlphaComposite", SurfaceOutput::Opacity, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.5f}},
    {"M_Blend_AlphaHoldout", SurfaceOutput::Opacity, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, {0.5f}},
    {"M_Blend_Modulate", SurfaceOutput::Emission, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenEmissive},
    //! UE 4.24 and older bake a view-dependent layer attenuation into the base color of a clear coat.
    {"M_SM_ClearCoat", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_25, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_ClearCoat", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_Cloth", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_Cloth", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_Eye", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_Eye", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_FromMaterialExpression", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_FromMaterialExpression", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_Hair", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_Hair", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_PreintegratedSkin", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_PreintegratedSkin", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_Subsurface", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_Subsurface", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_SubsurfaceProfile", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_SubsurfaceProfile", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
    {"M_SM_TwoSidedFoliage", SurfaceOutput::BaseColor, 3, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenBaseColor},
    {"M_SM_TwoSidedFoliage", SurfaceOutput::Roughness, 1, EGame::GAME_UE4_0, EGame::GAME_UE5_LATEST, refGenRoughness},
};

std::string surfacePath(std::string_view path) {
    if (path.starts_with("/"))
        return std::string(path);
    return std::format("{}{}.{}", catalog, path, path);
}

std::optional<uint32_t> surfaceOutput(const cv::MaterialProgram& program, const SurfaceOutput output) {
    switch (output) {
    case SurfaceOutput::BaseColor:
        return program.baseColor;
    case SurfaceOutput::Emission:
        return program.emission;
    case SurfaceOutput::Roughness:
        return program.roughness;
    case SurfaceOutput::Metallic:
        return program.metallic;
    case SurfaceOutput::Opacity:
        if (program.opacity < 0)
            return std::nullopt;
        return static_cast<uint32_t>(program.opacity);
    }
    return std::nullopt;
}

void checkConstantSurface(const Cook& cook, const cv::MaterialProgram& program, const std::string& path) {
    const ProgramValues values(program);
    for (const ConstantSurface& surface : constantSurfaces) {
        if (surfacePath(surface.path) != path || cook.game < surface.first || cook.game > surface.last)
            continue;
        const std::string what = describe(path, std::format("surface.{}", static_cast<int>(surface.output)));
        const std::optional<uint32_t> output = surfaceOutput(program, surface.output);
        if (!output) {
            fail(what, "is not written");
            continue;
        }
        const std::optional<Lanes> lanes = values.get(*output);
        if (!lanes) {
            fail(what, "is not constant");
            continue;
        }
        for (uint32_t lane = 0; lane < surface.components; ++lane)
            checkNear((*lanes)[lane], surface.value[lane], valueTolerance, std::format("{}.{}", what, lane));
    }
}

//! Converts the material the way UnrealKit does: the base pass pixel and vertex shaders to validated SPIR-V, then the surface to a material program and WGSL.
void convertMaterial(const Cook& cook, const rt::FShaderCodeLibrary& library, cv::SpirvCache& cache, const std::string& path, std::size_t& converted) {
    rt::UMaterialInterface* material = loadObject<rt::UMaterialInterface>(path);
    if (material == nullptr)
        return;
    Result<std::optional<cv::MaterialShaderMaps>> maps = cv::loadMaterialShaderMaps(*material);
    if (!maps)
        printError(path, maps.error());
    if (!CHECK(maps.has_value()))
        return;
    if (!maps->has_value()) {
        checkUncompiled(cook, *material, path);
        return;
    }
    if (isUncompiled(cook, path))
        fail(path, "is listed as a material whose shaders did not compile, but it has a shader map");
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
        shaderData.push_back(
            {code.typeName, stages[index], {reinterpret_cast<const uint8_t*>(code.code.data()), code.code.size()}, code.hash, code.uniformBufferNames});
    }
    const rt::FUniformExpressionSet& expressions = exported->shaderMap->Content.MaterialCompilationOutput.UniformExpressionSet;
    std::vector<cv::NumericParameter> parameters;
    for (const cv::MaterialNumericParameterValue& parameter : exported->numericParameters)
        parameters.push_back({keep(parameter.name),
                              parameter.type == rt::EMaterialParameterType::Vector,
                              {parameter.value.R, parameter.value.G, parameter.value.B, parameter.value.A}});
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
        textures.push_back({keep(slot.parameterName), slot.texture != nullptr ? keep(getPathName(slot.texture)) : std::string_view(), slot.type, slot.index,
                            slot.addressX, slot.addressY, slot.nearest, slot.size});
    std::vector<cv::TextureExtent> textureSizes;
    for (const cv::MaterialTextureExtent& extent : exported->textureExtents)
        textureSizes.push_back({keep(extent.parameterName), extent.referencedTextureIndex, extent.sizeX, extent.sizeY, extent.numSlices});
    std::vector<cv::ParameterCollection> collections;
    for (const rt::UMaterialParameterCollection* collection : exported->parameterCollections)
        collections.push_back(parameterCollection(collection, strings));
    const cv::BlendMode blend = blendMode(material->GetBlendMode().GetValue());
    const cv::MaterialData data{
        .owner = path,
        .blendMode = blend,
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
    ++converted;
    if (hasMaterialValues(cook))
        checkBlendMode(cook, *material, blend, path);
    checkNumericParameters(cook, *exported, translated->program, path);
    checkTextureSlots(cook, *exported, translated->program, path);
    checkPreshaders(data, path);
    checkConstantSurface(cook, translated->program, path);
}

void checkConversion(const Cook& cook) {
    const std::vector<std::string> materials = findConvertedMaterials(cook);
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
        std::size_t converted = 0;
        for (const std::string& path : materials)
            convertMaterial(cook, library, cache, path, converted);
        std::printf("material.convert: %zu of %zu materials converted\n", converted, materials.size());
    });
}

}

void addMaterialConvertChecks(Checks& checks) {
    checks.cook.emplace("material.convert", checkConversion);
}

}
