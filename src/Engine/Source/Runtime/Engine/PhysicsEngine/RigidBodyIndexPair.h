#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"

namespace ue5parse::runtime {

struct FRigidBodyIndexPair {
    int32 Indices[2] = {0, 0};

    bool operator==(const FRigidBodyIndexPair& Other) const { return Indices[0] == Other.Indices[0] && Indices[1] == Other.Indices[1]; }
    friend uint32 GetTypeHash(const FRigidBodyIndexPair& Pair) { return HashCombine(static_cast<uint32>(Pair.Indices[0]), static_cast<uint32>(Pair.Indices[1])); }
    friend FArchive& operator<<(FArchive& Ar, FRigidBodyIndexPair& Pair) {
        Ar << Pair.Indices[0] << Pair.Indices[1];
        return Ar;
    }
};

}
