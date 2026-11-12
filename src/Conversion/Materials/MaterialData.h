#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"

namespace ue5parse::conversion {

enum class BlendMode { Opaque, Masked, Translucent, Additive, Modulate, AlphaComposite, AlphaHoldout };

enum class ShaderStage { Pixel, Vertex };

enum class VertexFactory : uint8_t { Mesh, ParticleSprite, ParticleMesh };

constexpr bool programSamples(const runtime::EMaterialTextureParameterType type) {
    return type != runtime::EMaterialTextureParameterType::Cube;
}

struct ShaderData {
    std::string_view type;
    ShaderStage stage = ShaderStage::Pixel;
    std::span<const uint8_t> code;
    runtime::FSHAHash hash;
};

struct NumericParameter {
    std::string_view name;
    bool vector = false;
    std::array<float, 4> value{};
};

inline constexpr std::string_view particleColor = "ParticleColor";
inline constexpr std::string_view particleRelativeTime = "ParticleRelativeTime";
inline constexpr std::string_view dynamicParameter = "DynamicParameter";
inline constexpr std::string_view particleSubImageIndex = "ParticleSubImageIndex";
inline constexpr std::string_view particleSubImageSize = "ParticleSubImageSize";
inline constexpr std::array<NumericParameter, 5> particleParameters{{
    {particleColor, true, {1.0f, 1.0f, 1.0f, 1.0f}},
    {particleRelativeTime, false, {}},
    {dynamicParameter, true, {1.0f, 1.0f, 1.0f, 1.0f}},
    {particleSubImageIndex, false, {}},
    {particleSubImageSize, true, {1.0f, 1.0f, 1.0f, 1.0f}},
}};

struct ParameterCollection {
    std::vector<NumericParameter> scalars;
    std::vector<NumericParameter> vectors;
};

inline std::string collectionParameterName(const std::string_view collection, const std::string_view parameter) {
    return std::string(collection) + "." + std::string(parameter);
}

struct PreshaderProgram {
    uint32_t opcodeOffset = 0;
    uint32_t opcodeSize = 0;
    uint32_t fieldIndex = 0;
    uint32_t fieldCount = 0;
};

struct PreshaderField {
    uint32_t bufferOffset = 0;
    uint32_t componentIndex = 0;
    int type = 0;
};

struct TextureSlot {
    std::string_view name;
    std::string_view texture;
    runtime::EMaterialTextureParameterType type = runtime::EMaterialTextureParameterType::Standard2D;
    int index = 0;
    int addressU = 0;
    int addressV = 0;
    bool nearest = false;
    std::array<int, 3> size{};
};

struct TextureExtent {
    std::string_view name;
    int index = 0;
    int width = 0;
    int height = 0;
    int depth = 0;
};

struct MaterialData {
    std::string_view owner;
    BlendMode blendMode = BlendMode::Opaque;
    EGame game = EGame::GAME_UE5_LATEST;
    std::span<const ShaderData> shaders;
    std::span<const NumericParameter> parameters;
    std::span<const PreshaderProgram> preshaders;
    std::span<const PreshaderField> fields;
    std::span<const uint8_t> preshaderCode;
    std::span<const std::string_view> preshaderNames;
    const runtime::FUniformExpressionSet* uniformExpressions = nullptr;
    std::span<const TextureSlot> textures;
    std::span<const TextureExtent> textureSizes;
    std::span<const ParameterCollection> parameterCollections;
    VertexFactory vertexFactory = VertexFactory::Mesh;
};

}
