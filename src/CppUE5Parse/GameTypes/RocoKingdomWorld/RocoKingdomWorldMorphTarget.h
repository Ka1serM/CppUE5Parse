#pragma once

#include <cstdint>

#include "CppUE5Parse/GameTypes/SupermassiveGames/DarkPicturesMorphTarget.h"

namespace ue5parse {

//! Roco Kingdom: World packs a morph delta into 64 bits: three 8-bit position values, a 17-bit tangent of 7-bit values and a 19-bit vertex index.
DecodedMorphTargetDelta decodeRocoKingdomWorldMorphTargetDelta(std::uint64_t packed);

}
