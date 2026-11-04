#pragma once

#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"

namespace ue5parse::runtime {

struct FPackedNormal {
    uint32 Packed = 0;

    FVector4f ToFVector4f() const;
};

struct FPackedRGBA16N {
    uint16 X = 0;
    uint16 Y = 0;
    uint16 Z = 0;
    uint16 W = 0;

    FVector4f ToFVector4f() const;
};

float HalfToFloat(uint16 Half);

}
