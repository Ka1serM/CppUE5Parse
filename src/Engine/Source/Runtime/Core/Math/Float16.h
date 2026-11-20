#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FFloat16 {
public:
    uint16 Encoded = 0;

    friend FArchive& operator<<(FArchive& Ar, FFloat16& V) { return Ar << V.Encoded; }
};

}
