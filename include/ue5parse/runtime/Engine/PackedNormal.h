#pragma once

#include "ue5parse/runtime/Core/Math/UnrealMath.h"

// Packed tangent basis vectors (UE's RenderCore PackedNormal.h): four signed bytes or four signed shorts. UE5 stores
// them with the sign bit flipped, which Unpack undoes.

namespace ue5parse::runtime {

struct FPackedNormal {
    uint32 Packed = 0;

    // The vector in [-1, 1]; W is the sign of the binormal.
    FVector4f ToFVector4f() const;
    FVector3f ToFVector3f() const {
        FVector4f V = ToFVector4f();
        return FVector3f(V.X, V.Y, V.Z);
    }
};

struct FPackedRGBA16N {
    uint16 X = 0;
    uint16 Y = 0;
    uint16 Z = 0;
    uint16 W = 0;

    FVector4f ToFVector4f() const;
};

// A half-precision float as a float (UE's FFloat16).
float HalfToFloat(uint16 Half);

}
