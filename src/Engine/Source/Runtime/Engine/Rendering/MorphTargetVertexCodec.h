#pragma once

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime::UE::MorphTargetVertexCodec {

struct FDeltaBatchHeader {
    uint32 DataOffset = 0;
    uint32 NumElements = 0;
    bool bTangents = false;
    int8 IndexBits = 0;
    TIntVector3<int8> PositionBits;
    TIntVector3<int8> TangentZBits;
    uint32 IndexMin = 0;
    FIntVector PositionMin;
    FIntVector TangentZMin;

    friend FArchive& operator<<(FArchive& Ar, FDeltaBatchHeader& InHeader) {
        Ar << InHeader.DataOffset << InHeader.NumElements << InHeader.bTangents << InHeader.IndexBits;
        Ar << InHeader.PositionBits.X << InHeader.PositionBits.Y << InHeader.PositionBits.Z;
        Ar << InHeader.TangentZBits.X << InHeader.TangentZBits.Y << InHeader.TangentZBits.Z;
        Ar << InHeader.IndexMin << InHeader.PositionMin << InHeader.TangentZMin;
        return Ar;
    }
};

}
