#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FMultiSizeIndexContainer {
public:
    void Serialize(FArchive& Ar);
    void Init(TArray<uint32>&& InIndices, uint8 InDataTypeSize) {
        DataTypeSize = InDataTypeSize;
        Indices = std::move(InIndices);
    }
    int32 Num() const { return Indices.Num(); }
    uint32 Get(int32 Index) const { return Indices[Index]; }
    const TArray<uint32>& GetIndices() const { return Indices; }

private:
    uint8 DataTypeSize = 2;
    TArray<uint32> Indices;
};

}
