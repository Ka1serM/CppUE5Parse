#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "CppUE5Parse/Core/Versions.h"

namespace ue5parse::conversion {

struct ShaderParameterLayoutMember {
    std::string_view name;
    std::uint32_t offset = 0;
};

struct ShaderParameterBufferLayout {
    std::span<const ShaderParameterLayoutMember> members;
    std::span<const std::string_view> resources;
};

struct EngineUniformBuffer {
    std::string_view name;
    ShaderParameterBufferLayout layout;
};

struct EngineShaderParameterLayouts {
    EGame game;
    std::span<const EngineUniformBuffer> buffers;
    std::span<const ShaderParameterLayoutMember> primitiveRecord;
};

const EngineShaderParameterLayouts& getEngineShaderParameterLayouts(EGame game);

}
