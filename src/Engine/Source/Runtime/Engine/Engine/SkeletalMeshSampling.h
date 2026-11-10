#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "SkeletalMeshSampling.generated.h"

namespace ue5parse::runtime {

struct FWeightedRandomSampler {
    TArray<float> Prob;
    TArray<int32> Alias;
    float TotalWeight = 0;

    friend FArchive& operator<<(FArchive& Ar, FWeightedRandomSampler& Sampler) { return Ar << Sampler.Prob << Sampler.Alias << Sampler.TotalWeight; }
};

USTRUCT()
struct FSkeletalMeshSamplingLODBuiltData {
    GENERATED_BODY()
    FWeightedRandomSampler AreaWeightedTriangleSampler;

    bool Serialize(FArchive& Ar) {
        Ar << AreaWeightedTriangleSampler;
        return true;
    }
};

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
