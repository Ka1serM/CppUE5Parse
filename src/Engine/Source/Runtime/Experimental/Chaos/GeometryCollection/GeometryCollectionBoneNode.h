#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FGeometryCollectionBoneNode {
    int32 Level = INDEX_NONE;
    int32 Parent = INDEX_NONE;
    TSet<int32> Children;
    uint32 StatusFlags = 0;

    friend FArchive& operator<<(FArchive& Ar, FGeometryCollectionBoneNode& Node) { return Ar << Node.Level << Node.Parent << Node.Children << Node.StatusFlags; }
};

}
