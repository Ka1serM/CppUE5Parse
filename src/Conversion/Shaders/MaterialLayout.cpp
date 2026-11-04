#include "Conversion/Shaders/MaterialLayout.h"

#include <algorithm>

namespace ue5parse::conversion {
namespace {

constexpr int materialCacheLayers = 8;
constexpr std::uint32_t indirectVirtualTextureUniformSize = 64;
constexpr std::uint32_t uint4Size = 16;

}

MaterialLayout::MaterialLayout(const runtime::FUniformExpressionSet& expressions, const EGame game) {
    using enum EGame;
    using enum runtime::EMaterialTextureParameterType;
    const auto count = [&](const runtime::EMaterialTextureParameterType type) {
        return expressions.UniformTextureParameters[static_cast<std::size_t>(type)].Num();
    };
    const int stacks = expressions.VTStacks.Num();
    const auto& textureCollections = expressions.UniformTextureCollectionParameters;
    const int collections = textureCollections.Num();
    const int virtualCollections =
        static_cast<int>(std::ranges::count_if(textureCollections, [](const auto& collection) { return collection.bIsVirtualCollection; }));
    const int cacheTagStacks = expressions.MaterialCacheTagStacks.Num();

    std::uint32_t offset = 0;
    const auto constant = [&](const char* name, const int elements, const std::uint32_t stride) {
        if (elements == 0)
            return;
        constants.push_back({name, offset});
        offset += static_cast<std::uint32_t>(elements) * stride;
    };
    if (game >= GAME_UE5_7 && virtualCollections > 0) {
        constants.push_back({"TextureCollectionPackedUniforms", offset});
        offset += static_cast<std::uint32_t>(game >= GAME_UE5_8 ? virtualCollections : collections) * indirectVirtualTextureUniformSize;
    }
    constant("VTPackedPageTableUniform", stacks, 2 * uint4Size);
    if (game >= GAME_UE5_7)
        constant("MaterialCacheTagPageTableUniform", cacheTagStacks, uint4Size);
    constant("VTPackedUniform", count(Virtual), uint4Size);
    if (game >= GAME_UE5_3)
        constant("SVTPackedUniform", count(SparseVolume), 2 * uint4Size);
    constant("PreshaderBuffer", static_cast<int>(expressions.UniformPreshaderBufferSize), uint4Size);

    const auto textureWithSampler = [&](const std::string& prefix, const int elements) {
        for (int i = 0; i < elements; ++i) {
            resources.push_back(prefix + "_" + std::to_string(i));
            resources.push_back(prefix + "_" + std::to_string(i) + "Sampler");
        }
    };
    const auto numbered = [&](const char* prefix, const int i) { return std::string(prefix) + "_" + std::to_string(i); };
    textureWithSampler("Texture2D", count(Standard2D));
    textureWithSampler("TextureCube", count(Cube));
    textureWithSampler("Texture2DArray", count(Array2D));
    textureWithSampler("TextureCubeArray", count(ArrayCube));
    textureWithSampler("VolumeTexture", count(Volume));
    if (game >= GAME_UE5_3)
        for (int i = 0; i < count(SparseVolume); ++i) {
            resources.push_back(numbered("SparseVolumeTexturePageTable", i));
            resources.push_back(numbered("SparseVolumeTexturePhysicalA", i));
            resources.push_back(numbered("SparseVolumeTexturePhysicalB", i));
            if (game < GAME_UE5_4)
                resources.push_back(numbered("SparseVolumeTextureStreamingInfoBuffer", i));
            resources.push_back(numbered("SparseVolumeTexturePhysical", i) + "Sampler");
        }
    textureWithSampler("ExternalTexture", expressions.UniformExternalTextureParameters.Num());
    if (game >= GAME_UE5_5)
        for (int i = 0; i < collections; ++i) {
            resources.push_back(numbered("TextureCollection", i));
            if (!textureCollections[i].bIsVirtualCollection)
                continue;
            resources.push_back(numbered("TextureCollectionPageTable", i));
            resources.push_back(numbered("TextureCollectionPhysical", i));
            resources.push_back(numbered("TextureCollectionSampler", i));
        }
    for (int i = 0; i < stacks; ++i) {
        resources.push_back(numbered("VirtualTexturePageTable0", i));
        if (expressions.VTStacks[i].NumLayers > 4)
            resources.push_back(numbered("VirtualTexturePageTable1", i));
        resources.push_back(numbered("VirtualTexturePageTableIndirection", i));
    }
    if (game >= GAME_UE5_7)
        for (int tag = 0; tag < cacheTagStacks; ++tag) {
            resources.push_back(numbered("MaterialCacheTagBuffer", tag));
            resources.push_back(numbered("MaterialCacheTagPageTable0", tag));
            for (int layer = 0; layer < materialCacheLayers; ++layer)
                resources.push_back(numbered("MaterialCacheTagPhysical", tag * materialCacheLayers + layer));
        }
    textureWithSampler("VirtualTexturePhysical", count(Virtual));
    resources.push_back("Wrap_WorldGroupSettings");
    resources.push_back("Clamp_WorldGroupSettings");
}

}
