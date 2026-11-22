#include "CppUE5Parse/GameTypes/RocoKingdomWorld/RocoKingdomWorldMorphTarget.h"

namespace ue5parse {

DecodedMorphTargetDelta decodeRocoKingdomWorldMorphTargetDelta(std::uint64_t packed) {
    const auto position = static_cast<std::uint32_t>(packed & 0xFFFFFFu);
    const auto tangent = static_cast<std::uint32_t>((packed >> 24) & 0x1FFFFu);
    DecodedMorphTargetDelta delta;
    for (int axis = 0; axis < 3; axis++) {
        delta.positionDelta[axis] = static_cast<float>(static_cast<std::int8_t>((position >> (axis * 8)) & 0xFFu)) / 127.5f;
        //! The 7-bit value is read as a signed byte, so it is never negative.
        delta.tangentZDelta[axis] = static_cast<float>(static_cast<std::int8_t>((tangent >> (axis * 7)) & 0x7Fu)) / 63.5f;
    }
    delta.sourceIndex = static_cast<std::uint32_t>(packed >> 45);
    return delta;
}

}
