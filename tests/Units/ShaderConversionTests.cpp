#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include "Conversion/Materials/MaterialShaderExport.h"
#include "Conversion/Shaders/EngineShaderParameterLayouts.h"
#include "Conversion/Spirv/SpirvEvaluator.h"
#include "Engine/Source/Runtime/RenderCore/Shader.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::conversion::EngineShaderParameterLayouts;
using ue5parse::conversion::EngineUniformBuffer;
using ue5parse::conversion::getEngineShaderParameterLayouts;
using ue5parse::conversion::halfToFloat;

const EngineUniformBuffer* findBuffer(const EngineShaderParameterLayouts& layouts, const std::string_view name) {
    const auto found = std::ranges::find(layouts.buffers, name, &EngineUniformBuffer::name);
    return found != layouts.buffers.end() ? &*found : nullptr;
}

bool hasMember(const EngineUniformBuffer& buffer, const std::string_view name) {
    return std::ranges::any_of(buffer.layout.members, [&](const auto& member) { return member.name == name; });
}

constexpr EGame releases[] = {
    EGame::GAME_UE4_0,  EGame::GAME_UE4_1,  EGame::GAME_UE4_2,  EGame::GAME_UE4_3,  EGame::GAME_UE4_4,  EGame::GAME_UE4_5,  EGame::GAME_UE4_6,
    EGame::GAME_UE4_7,  EGame::GAME_UE4_8,  EGame::GAME_UE4_9,  EGame::GAME_UE4_10, EGame::GAME_UE4_11, EGame::GAME_UE4_12, EGame::GAME_UE4_13,
    EGame::GAME_UE4_14, EGame::GAME_UE4_15, EGame::GAME_UE4_16, EGame::GAME_UE4_17, EGame::GAME_UE4_18, EGame::GAME_UE4_19, EGame::GAME_UE4_20,
    EGame::GAME_UE4_21, EGame::GAME_UE4_22, EGame::GAME_UE4_23, EGame::GAME_UE4_24, EGame::GAME_UE4_25, EGame::GAME_UE4_26, EGame::GAME_UE4_27,
    EGame::GAME_UE5_0,  EGame::GAME_UE5_1,  EGame::GAME_UE5_2,  EGame::GAME_UE5_3,  EGame::GAME_UE5_4,  EGame::GAME_UE5_5,  EGame::GAME_UE5_6,
    EGame::GAME_UE5_7,  EGame::GAME_UE5_8,  EGame::GAME_UE5_9,
};

}

TEST(halfToFloatDecodesEveryClass) {
    CHECK(halfToFloat(0x3C00u) == 1.0f);
    CHECK(halfToFloat(0xC000u) == -2.0f);
    CHECK(halfToFloat(0x3555u) == 0.333251953125f);
    CHECK(halfToFloat(0x7BFFu) == 65504.0f);
    CHECK(halfToFloat(0x0400u) == std::ldexp(1.0f, -14));
    CHECK(halfToFloat(0x0001u) == std::ldexp(1.0f, -24));
    CHECK(halfToFloat(0x03FFu) == std::ldexp(1023.0f, -24));
    CHECK(halfToFloat(0x0000u) == 0.0f && !std::signbit(halfToFloat(0x0000u)));
    CHECK(halfToFloat(0x8000u) == 0.0f && std::signbit(halfToFloat(0x8000u)));
    CHECK(std::isinf(halfToFloat(0x7C00u)) && halfToFloat(0x7C00u) > 0.0f);
    CHECK(std::isinf(halfToFloat(0xFC00u)) && halfToFloat(0xFC00u) < 0.0f);
    CHECK(std::isnan(halfToFloat(0x7E00u)));
}

TEST(engineLayoutsPerRelease) {
    for (const EGame game : releases) {
        const EngineShaderParameterLayouts* layouts = getEngineShaderParameterLayouts(game);
        if (!CHECK(layouts != nullptr && layouts->game == game))
            continue;
        const EngineUniformBuffer* view = findBuffer(*layouts, "View");
        CHECK(view != nullptr && view->structName == "FViewUniformShaderParameters" && hasMember(*view, "ViewToClip"));
        CHECK(findBuffer(*layouts, "Primitive") != nullptr);
        CHECK((findBuffer(*layouts, "InstanceCulling") != nullptr) == (game >= EGame::GAME_UE5_0));
        CHECK((findBuffer(*layouts, "Scene") != nullptr) == (game >= EGame::GAME_UE5_3));
        CHECK(layouts->primitiveRecord.empty() == (game < EGame::GAME_UE4_25));
    }
}

TEST(engineLayoutsBetweenReleases) {
    const EngineShaderParameterLayouts* layouts = getEngineShaderParameterLayouts(EGame::GAME_Valorant);
    CHECK(layouts != nullptr && layouts->game <= EGame::GAME_Valorant);
}

TEST(engineLayoutsUE5Records) {
    const EngineShaderParameterLayouts* ue4 = getEngineShaderParameterLayouts(EGame::GAME_UE4_27);
    REQUIRE(ue4 != nullptr && !ue4->primitiveRecord.empty());
    CHECK(ue4->primitiveRecord.front().name == "LocalToWorld" && ue4->primitiveRecord.front().offset == 0u);
    for (const EGame game : {EGame::GAME_UE5_0, EGame::GAME_UE5_5, EGame::GAME_UE5_9}) {
        const EngineShaderParameterLayouts* layouts = getEngineShaderParameterLayouts(game);
        REQUIRE(layouts != nullptr && !layouts->primitiveRecord.empty());
        //! FPrimitiveSceneData of UE5 starts with the primitive flags.
        CHECK(layouts->primitiveRecord.front().name == "Flags" && layouts->primitiveRecord.front().offset == 0u);
        CHECK(std::ranges::is_sorted(layouts->primitiveRecord, {}, [](const auto& member) { return member.offset; }));
    }
}

TEST(engineLayoutsSceneMembers) {
    //! TExtensibleUniformBuffer sorts its members by name, so CurveSkinning comes before GPUScene from UE 5.9.
    const EngineShaderParameterLayouts* ue58 = getEngineShaderParameterLayouts(EGame::GAME_UE5_8);
    const EngineShaderParameterLayouts* ue59 = getEngineShaderParameterLayouts(EGame::GAME_UE5_9);
    REQUIRE(ue58 != nullptr && ue59 != nullptr);
    const EngineUniformBuffer* scene58 = findBuffer(*ue58, "Scene");
    const EngineUniformBuffer* scene59 = findBuffer(*ue59, "Scene");
    REQUIRE(scene58 != nullptr && scene59 != nullptr);
    CHECK(scene58->layout.resources.front().starts_with("GPUScene_"));
    CHECK(scene59->layout.resources.front().starts_with("CurveSkinning_"));
    CHECK(scene58->layout.members.front().name == "GPUScene_GPUSceneInstanceDataTileSizeLog2" && scene58->layout.members.front().offset == 0u);
    CHECK(scene59->layout.members.front().name == "GPUScene_GPUSceneInstanceDataTileSizeLog2" && scene59->layout.members.front().offset == 32u);
}

TEST(legacyUniformBufferNames) {
    using namespace ue5parse::runtime;
    FShader shader;
    shader.MaterialUniformBuffer.BaseIndex = 2;
    shader.ParameterCollectionUniformBuffers.Add(FShaderUniformBufferParameter{3});
    shader.ParameterCollectionUniformBuffers.Add(FShaderUniformBufferParameter{FShaderUniformBufferParameter::UnboundIndex});
    shader.UniformBufferParameters.Add(FShaderUniformBufferParameter{0});
    shader.UniformBufferParameterStructNames.Add(ue5parse::FName("FViewUniformShaderParameters"));
    shader.UniformBufferParameters.Add(FShaderUniformBufferParameter{1});
    shader.UniformBufferParameterStructNames.Add(ue5parse::FName("FPrimitiveUniformShaderParameters"));
    const auto names = ue5parse::conversion::getLegacyUniformBufferNames(shader, EGame::GAME_UE4_10);
    REQUIRE(names.has_value());
    CHECK((*names == std::vector<std::string>{"View", "Primitive", "Material", "MaterialCollection0"}));

    const auto current = ue5parse::conversion::getLegacyUniformBufferNames(shader, EGame::GAME_UE4_11);
    CHECK(current.has_value() && current->empty());

    shader.UniformBufferParameterStructNames[1] = ue5parse::FName("FUnknownUniformShaderParameters");
    CHECK(!ue5parse::conversion::getLegacyUniformBufferNames(shader, EGame::GAME_UE4_10).has_value());
}
