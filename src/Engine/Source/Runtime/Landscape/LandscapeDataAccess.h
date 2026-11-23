#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"

#define LANDSCAPE_ZSCALE (1.0f / 128.0f)

namespace ue5parse::runtime::LandscapeDataAccess {

inline constexpr float MidValue = 32768.f;

inline float GetLocalHeight(uint16 Height) {
    return (static_cast<float>(Height) - MidValue) * LANDSCAPE_ZSCALE;
}

}
