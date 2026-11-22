#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

struct DecodedMorphTargetDelta {
    std::array<float, 3> positionDelta{};
    std::array<float, 3> tangentZDelta{};
    std::uint32_t sourceIndex = 0;
};

struct DarkPicturesMorphTargetDeltas {
    std::vector<DecodedMorphTargetDelta> deltas;
    std::size_t bytesRead = 0;
};

//! The Dark Pictures Anthology stores morph deltas in batches of 8-bit indices and half vectors, with a half scale except in Little Hope.
Result<DarkPicturesMorphTargetDeltas> decodeDarkPicturesMorphTargetDeltas(std::span<const std::byte> data, bool bScaledHalfVectors);

}
