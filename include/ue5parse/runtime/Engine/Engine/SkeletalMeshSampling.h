#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "SkeletalMeshSampling.generated.h"

namespace ue5parse::runtime {

// FWeightedRandomSampler: the alias table of an area-weighted triangle sampler (UE's WeightedRandomSampler.h).
struct FWeightedRandomSampler {
    TArray<float> Prob;
    TArray<int32> Alias;
    float TotalWeight = 0;

    friend FArchive& operator<<(FArchive& Ar, FWeightedRandomSampler& Sampler) {
        return Ar << Sampler.Prob << Sampler.Alias << Sampler.TotalWeight;
    }
};

// FSkeletalMeshSamplingLODBuiltData: the triangle sampler of a skeletal mesh LOD, for particles (UE's
// Engine/SkeletalMeshSampling.h). Cooked data writes it natively.
USTRUCT()
struct FSkeletalMeshSamplingLODBuiltData {
    GENERATED_BODY()
    FWeightedRandomSampler AreaWeightedTriangleSampler;

    bool Serialize(FArchive& Ar) {
        Ar << AreaWeightedTriangleSampler;
        return true;
    }
};

// FSkeletalMeshSamplingRegionBuiltData: the triangles, bones and vertices of a sampling region, and its sampler.
USTRUCT()
struct FSkeletalMeshSamplingRegionBuiltData {
    GENERATED_BODY()
    TArray<int32> TriangleIndices;
    TArray<int32> BoneIndices;
    FWeightedRandomSampler AreaWeightedSampler;
    TArray<int32> Vertices;

    bool Serialize(FArchive& Ar) {
        Ar << TriangleIndices << BoneIndices << AreaWeightedSampler << Vertices;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FSkeletalMeshSamplingLODBuiltData> : TStructOpsTypeTraitsBase2<FSkeletalMeshSamplingLODBuiltData> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FSkeletalMeshSamplingRegionBuiltData> : TStructOpsTypeTraitsBase2<FSkeletalMeshSamplingRegionBuiltData> {
    enum { WithSerializer = true };
};

}
