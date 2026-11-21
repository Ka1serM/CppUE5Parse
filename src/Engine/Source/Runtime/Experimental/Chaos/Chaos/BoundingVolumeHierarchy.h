#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"

namespace ue5parse::runtime::Chaos {

template <class T>
struct TBVHNode {
    TVector<T> MMin;
    TVector<T> MMax;
    int32 MAxis = 0;
    TArray<int32> MChildren;
    int32 LeafIndex = 0;

    friend FArchive& operator<<(FArchive& Ar, TBVHNode& Node) {
        Ar << Node.LeafIndex << Node.MAxis << Node.MChildren;
        SerializeReal(Ar, Node.MMax);
        return SerializeReal(Ar, Node.MMin);
    }
};

template <class LEAF_TYPE, class T>
class TBoundingVolumeHierarchy {
public:
    void Serialize(FArchive& Ar);

    const TArray<int32>& GetGlobalObjects() const { return MGlobalObjects; }
    const TMap<int32, TAABB<T>>& GetWorldSpaceBoxes() const { return MWorldSpaceBoxes; }
    int32 GetMaxLevels() const { return MMaxLevels; }
    const TArray<TBVHNode<T>>& GetElements() const { return Elements; }
    const TArray<LEAF_TYPE>& GetLeafs() const { return Leafs; }

private:
    TArray<int32> MGlobalObjects;
    TMap<int32, TAABB<T>> MWorldSpaceBoxes;
    int32 MMaxLevels = 12;
    TArray<TBVHNode<T>> Elements;
    TArray<LEAF_TYPE> Leafs;
};

}
