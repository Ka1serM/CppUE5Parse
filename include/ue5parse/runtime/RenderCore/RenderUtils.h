#pragma once

#include <cmath>

// The exposure conversions of UE's RenderCore RenderUtils.h, with a maximum luminance of 1.

namespace ue5parse::runtime {

inline float EV100ToLuminance(float EV100) { return std::pow(2.0f, EV100); }
inline float LuminanceToEV100(float Luminance) { return std::log2(Luminance); }

}
