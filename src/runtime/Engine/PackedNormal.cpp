#include "ue5parse/runtime/Engine/PackedNormal.h"

#include <cstring>

namespace ue5parse::runtime {

FVector4f FPackedNormal::ToFVector4f() const {
    // The sign bit of each byte is flipped in UE5 data.
    const uint32 Data = Packed ^ 0x80808080u;
    auto Unpack = [Data](int32 Shift) { return static_cast<float>((Data >> Shift) & 0xFF) / 127.5f - 1.0f; };
    return FVector4f(Unpack(0), Unpack(8), Unpack(16), Unpack(24));
}

FVector4f FPackedRGBA16N::ToFVector4f() const {
    auto Unpack = [](uint16 Value) { return (static_cast<float>(Value ^ 0x8000) - 32767.5f) / 32767.5f; };
    return FVector4f(Unpack(X), Unpack(Y), Unpack(Z), Unpack(W));
}

float HalfToFloat(uint16 Half) {
    uint32 Sign = static_cast<uint32>(Half & 0x8000) << 16;
    uint32 Exponent = (Half >> 10) & 0x1F;
    uint32 Mantissa = Half & 0x3FF;
    uint32 Bits;
    if (Exponent == 0) {
        if (Mantissa == 0) {
            Bits = Sign;
        } else {
            // Subnormal: normalize.
            Exponent = 127 - 15 + 1;
            while ((Mantissa & 0x400) == 0) {
                Mantissa <<= 1;
                Exponent--;
            }
            Mantissa &= 0x3FF;
            Bits = Sign | (Exponent << 23) | (Mantissa << 13);
        }
    } else if (Exponent == 31) {
        Bits = Sign | 0x7F800000u | (Mantissa << 13);
    } else {
        Bits = Sign | ((Exponent - 15 + 127) << 23) | (Mantissa << 13);
    }
    float Value;
    std::memcpy(&Value, &Bits, 4);
    return Value;
}

}
