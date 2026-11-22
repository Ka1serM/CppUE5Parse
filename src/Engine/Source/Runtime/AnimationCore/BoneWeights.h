#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FBoneWeight {
    uint16 BoneIndex = 0;
    uint16 RawWeight = 0;
};

inline FArchive& operator<<(FArchive& Ar, FBoneWeight& BoneWeight) {
    return Ar << BoneWeight.BoneIndex << BoneWeight.RawWeight;
}

}
