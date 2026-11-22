#pragma once

#include <bit>
#include <cmath>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FFloat16 {
public:
    uint16 Encoded = 0;

    float GetFloat() const {
        const uint32 Sign = static_cast<uint32>(Encoded & 0x8000u) << 16;
        const uint32 Exponent = (Encoded >> 10) & 0x1Fu;
        const uint32 Mantissa = Encoded & 0x3FFu;
        if (Exponent == 0) {
            const float Value = std::ldexp(static_cast<float>(Mantissa), -24);
            return Sign != 0 ? -Value : Value;
        }
        if (Exponent == 31)
            return std::bit_cast<float>(Sign | 0x7F800000u | (Mantissa << 13));
        return std::bit_cast<float>(Sign | ((Exponent + 112) << 23) | (Mantissa << 13));
    }

    friend FArchive& operator<<(FArchive& Ar, FFloat16& V) { return Ar << V.Encoded; }
};

}
