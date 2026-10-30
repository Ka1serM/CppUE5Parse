#pragma once

#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

// FRawStaticIndexBuffer: 16-bit or 32-bit indices, widened to 32 bits.
class FRawStaticIndexBuffer {
public:
    void Serialize(FArchive& Ar);
    int32 GetNumIndices() const { return Indices.Num(); }
    bool Is32Bit() const { return b32Bit; }
    uint32 GetIndex(int32 At) const { return Indices[At]; }
    const TArray<uint32>& GetIndices() const { return Indices; }

private:
    bool b32Bit = false;
    TArray<uint32> Indices;
};

}
