#pragma once

#include <cstdint>

namespace ue5parse::conversion {

//! D3D gives each stage its own registers, and D3D has at most 128 of each, so non-pixel stages start their bindings at 128.
inline constexpr std::uint32_t stageBindings = 128;

}
