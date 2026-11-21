#pragma once

#include "Engine/Source/Runtime/GeometryCore/Util/DynamicVector.h"

namespace ue5parse::runtime {

class FRefCountVector {
public:
    void Serialize(FArchive& Ar) {
        bool bCompactData = false;
        bool bUseCompression = false;
        Ar << bCompactData << bUseCompression << UsedCount;
        RefCounts.Serialize(Ar);
        FreeIndices = {};
        if (UsedCount == RefCounts.Num() || bCompactData || bUseCompression)
            return;
        FreeIndices.Serialize(Ar);
    }

    int32 GetCount() const { return UsedCount; }
    const TDynamicVector<uint16>& GetRawRefCounts() const { return RefCounts; }

private:
    TDynamicVector<uint16> RefCounts;
    TDynamicVector<int32> FreeIndices;
    int32 UsedCount = 0;
};

}
