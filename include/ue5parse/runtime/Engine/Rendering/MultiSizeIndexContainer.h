#pragma once

#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

// FMultiSizeIndexContainer: the indices of a skeletal mesh LOD, 16-bit or 32-bit, widened to 32 bits.
class FMultiSizeIndexContainer {
public:
    void Serialize(FArchive& Ar);
    uint8 GetDataTypeSize() const { return DataTypeSize; }
    int32 Num() const { return Indices.Num(); }
    uint32 Get(int32 Index) const { return Indices[Index]; }
    const TArray<uint32>& GetIndices() const { return Indices; }

private:
    uint8 DataTypeSize = 2;
    TArray<uint32> Indices;
};

}
