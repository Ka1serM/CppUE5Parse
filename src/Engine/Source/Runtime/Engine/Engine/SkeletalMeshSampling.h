#pragma once

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
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
        Ar << TriangleIndices << BoneIndices << AreaWeightedSampler;
        if (Ar.CustomVerAtLeast(FNiagaraObjectVersion::GUID, FNiagaraObjectVersion::SkeletalMeshVertexSampling, EGame::GAME_UE4_21))
            Ar << Vertices;
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

USTRUCT()
struct FSkeletalMeshSamplingBuiltData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FSkeletalMeshSamplingLODBuiltData> WholeMeshBuiltData;
    UPROPERTY()
    TArray<FSkeletalMeshSamplingRegionBuiltData> RegionBuiltData;
};

USTRUCT()
struct FSkeletalMeshSamplingRegionBoneFilter {
    GENERATED_BODY()
    UPROPERTY()
    FName BoneName;
    UPROPERTY()
    bool bIncludeOrExclude = true;
    UPROPERTY()
    bool bApplyToChildren = true;
};

USTRUCT()
struct FSkeletalMeshSamplingRegionMaterialFilter {
    GENERATED_BODY()
    UPROPERTY()
    FName MaterialName;
};

USTRUCT()
struct FSkeletalMeshSamplingRegion {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    int32 LODIndex = INDEX_NONE;
    UPROPERTY()
    bool bSupportUniformlyDistributedSampling = false;
    UPROPERTY()
    TArray<FSkeletalMeshSamplingRegionMaterialFilter> MaterialFilters;
    UPROPERTY()
    TArray<FSkeletalMeshSamplingRegionBoneFilter> BoneFilters;
};

USTRUCT()
struct FSkeletalMeshSamplingInfo {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FSkeletalMeshSamplingRegion> Regions;
    UPROPERTY()
    FSkeletalMeshSamplingBuiltData BuiltData;
};

}
