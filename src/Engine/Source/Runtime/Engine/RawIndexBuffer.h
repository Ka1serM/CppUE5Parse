#pragma once

#include <algorithm>
#include <span>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FRawStaticIndexBuffer {
public:
    void Serialize(FArchive& Ar);
    int32 GetNumIndices() const { return Indices.Num(); }
    bool Is32Bit() const { return b32Bit; }
    uint32 GetIndex(int32 At) const { return Indices[At]; }
    const TArray<uint32>& GetIndices() const { return Indices; }
    void SetIndices(std::span<const uint32> InIndices) {
        Indices.SetNum(static_cast<int32>(InIndices.size()));
        std::copy(InIndices.begin(), InIndices.end(), Indices.GetData());
    }

private:
    bool b32Bit = false;
    TArray<uint32> Indices;
};

}
