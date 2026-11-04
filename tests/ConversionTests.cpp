#include <array>
#include <cstddef>
#include <cstdint>

#include "Conversion/Shaders/EngineShaderParameterLayouts.h"
#include "Conversion/Shaders/MaterialLayout.h"
#include "Conversion/Shaders/ShaderCompiler.h"
#include "Conversion/Sounds/SoundDecoder.h"
#include "Test.h"

using namespace ue5parse;
using namespace ue5parse::conversion;

TEST(engineLayoutsChooseTheNewestReleaseAtOrBeforeTheGame) {
    CHECK(getEngineShaderParameterLayouts(EGame::GAME_UE5_3).game == EGame::GAME_UE5_3);
    CHECK(getEngineShaderParameterLayouts(EGame::GAME_UE5_LATEST).game >= EGame::GAME_UE5_8);
}

TEST(materialLayoutOfAnEmptyExpressionSetHasOnlyTheSharedSamplers) {
    const MaterialLayout layout(runtime::FUniformExpressionSet{}, EGame::GAME_UE5_3);
    CHECK(layout.getConstants().empty());
    REQUIRE(layout.getResources().size() == 2);
    CHECK(layout.getResources()[0] == "Wrap_WorldGroupSettings");
    CHECK(layout.getResources()[1] == "Clamp_WorldGroupSettings");
}

TEST(materialLayoutPlacesConstantsByTheirStrides) {
    runtime::FUniformExpressionSet expressions;
    expressions.UniformPreshaderBufferSize = 3;
    expressions.VTStacks.Add(runtime::FMaterialVirtualTextureStack{.NumLayers = 5, .LayerUniformExpressionIndices = {}, .PreallocatedStackTextureIndex = 0});
    const MaterialLayout layout(expressions, EGame::GAME_UE5_3);
    REQUIRE(layout.getConstants().size() == 2);
    CHECK(layout.getConstants()[0].name == "VTPackedPageTableUniform");
    CHECK(layout.getConstants()[1].name == "PreshaderBuffer");
    CHECK(layout.getConstants()[1].offset == 32);
    CHECK(layout.getResources()[1] == "VirtualTexturePageTable1_0");
}

TEST(materialLayoutSizesVirtualCollectionUniformsByRelease) {
    runtime::FUniformExpressionSet expressions;
    expressions.UniformPreshaderBufferSize = 1;
    expressions.UniformTextureCollectionParameters.Add(runtime::FMaterialTextureCollectionParameterInfo{});
    runtime::FMaterialTextureCollectionParameterInfo virtualCollection;
    virtualCollection.bIsVirtualCollection = true;
    expressions.UniformTextureCollectionParameters.Add(virtualCollection);
    const MaterialLayout ue57(expressions, EGame::GAME_UE5_7);
    const MaterialLayout ue58(expressions, EGame::GAME_UE5_8);
    REQUIRE(ue57.getConstants().size() == 2);
    REQUIRE(ue58.getConstants().size() == 2);
    CHECK(ue57.getConstants()[0].name == "TextureCollectionPackedUniforms");
    CHECK(ue57.getConstants()[1].offset == 128);
    CHECK(ue58.getConstants()[1].offset == 64);
    CHECK(ue57.getResources()[0] == "TextureCollection_0");
    CHECK(ue57.getResources()[1] == "TextureCollection_1");
    CHECK(ue57.getResources()[2] == "TextureCollectionPageTable_1");
}

TEST(materialLayoutIgnoresTextureCollectionsBeforeTheirRelease) {
    runtime::FUniformExpressionSet expressions;
    expressions.UniformTextureCollectionParameters.Add(runtime::FMaterialTextureCollectionParameterInfo{});
    CHECK(MaterialLayout(expressions, EGame::GAME_UE5_4).getResources().size() == 2);
    CHECK(MaterialLayout(expressions, EGame::GAME_UE5_5).getResources().size() == 3);
}

TEST(materialLayoutNamesTheCacheTagResourcesFromUE57) {
    runtime::FUniformExpressionSet expressions;
    expressions.MaterialCacheTagStacks.Add(runtime::FMaterialCacheTagStack{});
    const MaterialLayout layout(expressions, EGame::GAME_UE5_7);
    REQUIRE(layout.getConstants().size() == 1);
    CHECK(layout.getConstants()[0].name == "MaterialCacheTagPageTableUniform");
    REQUIRE(layout.getResources().size() == 12);
    CHECK(layout.getResources()[0] == "MaterialCacheTagBuffer_0");
    CHECK(layout.getResources()[9] == "MaterialCacheTagPhysical_7");
    CHECK(MaterialLayout(expressions, EGame::GAME_UE5_6).getConstants().empty());
}

TEST(compileShaderReportsTheCauseOfUnreadableCode) {
    const std::array<std::uint8_t, 4> code{};
    const MaterialLayout layout(runtime::FUniformExpressionSet{}, EGame::GAME_UE5_3);
    const auto compiled = compileShader(code, EGame::GAME_UE5_3, layout);
    REQUIRE(!compiled);
    CHECK(compiled.error().cause.has_value() || !compiled.error().message.empty());
}

TEST(decodeSoundRejectsAnUnknownFormat) {
    const std::array<std::byte, 16> file{};
    const auto sound = decodeSound(file, "unknown.wem");
    REQUIRE(!sound);
    CHECK(sound.error().message == "vgmstream does not recognise the sound 'unknown.wem'.");
}
