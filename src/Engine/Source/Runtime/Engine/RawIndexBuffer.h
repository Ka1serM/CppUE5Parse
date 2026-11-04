#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

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
