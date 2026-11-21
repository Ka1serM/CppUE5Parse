#include "Conversion/Shaders/EngineShaderParameterLayouts.h"

#include <algorithm>
#include <iterator>

namespace ue5parse::conversion {
namespace {

#include "Conversion/Shaders/EngineShaderParameterLayouts.inc"

}

const EngineShaderParameterLayouts* getEngineShaderParameterLayouts(const EGame game) {
    const auto newer = std::ranges::upper_bound(Releases, game, {}, &EngineShaderParameterLayouts::game);
    if (newer == std::begin(Releases))
        return nullptr;
    return &*std::prev(newer);
}

}
