#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FGeometryCollectionSection {
    int32 MaterialID = 0;
    int32 FirstIndex = 0;
    int32 NumTriangles = 0;
    int32 MinVertexIndex = 0;
    int32 MaxVertexIndex = 0;

    friend FArchive& operator<<(FArchive& Ar, FGeometryCollectionSection& Section) {
        return Ar << Section.MaterialID << Section.FirstIndex << Section.NumTriangles << Section.MinVertexIndex << Section.MaxVertexIndex;
    }
};

}
