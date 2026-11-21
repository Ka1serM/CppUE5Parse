#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime::MorphTargetVertexCodec {

struct FDeltaBatchHeader {
    uint32 DataOffset = 0;
    uint32 NumElements = 0;
    bool bTangents = false;
    int8 IndexBits = 0;
    TIntVector3<int8> PositionBits{};
    TIntVector3<int8> TangentZBits{};
    uint32 IndexMin = 0;
    FIntVector PositionMin{};
    FIntVector TangentZMin{};

    friend FArchive& operator<<(FArchive& Ar, FDeltaBatchHeader& H) {
        return Ar << H.DataOffset << H.NumElements << H.bTangents << H.IndexBits << H.PositionBits << H.TangentZBits << H.IndexMin << H.PositionMin
                  << H.TangentZMin;
    }
};

}
