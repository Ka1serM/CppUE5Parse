#pragma once

#include "ue5parse/runtime/AnimationCore/BoneIndices.h"
#include "ue5parse/runtime/Core/Containers/Map.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

// The bone hierarchy of a skeleton or mesh (UE's ReferenceSkeleton.h): the raw bones as imported, and the final bones
// with the skeleton's virtual bones appended.

namespace ue5parse::runtime {

class USkeleton;

struct FMeshBoneInfo {
    FName Name;
    int32 ParentIndex = INDEX_NONE;

    FMeshBoneInfo() = default;
    FMeshBoneInfo(FName InName, int32 InParentIndex) : Name(InName), ParentIndex(InParentIndex) {}
    bool operator==(const FMeshBoneInfo& Other) const { return Name == Other.Name; }
    friend FArchive& operator<<(FArchive& Ar, FMeshBoneInfo& F) { return Ar << F.Name << F.ParentIndex; }
};

// A virtual bone of the final bones: its index, and the raw bones it is made from.
struct FVirtualBoneRefData {
    int32 VBRefSkelIndex = INDEX_NONE;
    int32 SourceRefSkelIndex = INDEX_NONE;
    int32 TargetRefSkelIndex = INDEX_NONE;
};

struct FReferenceSkeleton {
public:
    // Builds the final bones from the raw bones and the skeleton's virtual bones (UE's RebuildRefSkeleton).
    void RebuildRefSkeleton(const USkeleton* Skeleton, bool bRebuildNameMap);

    int32 GetNum() const { return FinalRefBoneInfo.Num(); }
    int32 GetRawBoneNum() const { return RawRefBoneInfo.Num(); }
    const TArray<FBoneIndexType>& GetRequiredVirtualBones() const { return RequiredVirtualBones; }
    const TArray<FVirtualBoneRefData>& GetVirtualBoneRefData() const { return UsedVirtualBoneData; }
    const TArray<FMeshBoneInfo>& GetRefBoneInfo() const { return FinalRefBoneInfo; }
    const TArray<FTransform>& GetRefBonePose() const { return FinalRefBonePose; }
    const TArray<FMeshBoneInfo>& GetRawRefBoneInfo() const { return RawRefBoneInfo; }
    const TArray<FTransform>& GetRawRefBonePose() const { return RawRefBonePose; }
    const TMap<FName, int32>& GetRawNameToIndexMap() const { return RawNameToIndexMap; }
    void Empty();

    int32 FindBoneIndex(FName BoneName) const;
    int32 FindRawBoneIndex(FName BoneName) const;
    FName GetBoneName(int32 BoneIndex) const { return FinalRefBoneInfo[BoneIndex].Name; }
    int32 GetParentIndex(int32 BoneIndex) const { return FinalRefBoneInfo[BoneIndex].ParentIndex; }
    int32 GetRawParentIndex(int32 BoneIndex) const { return RawRefBoneInfo[BoneIndex].ParentIndex; }
    bool IsValidIndex(int32 Index) const { return FinalRefBoneInfo.IsValidIndex(Index); }
    bool IsValidRawIndex(int32 Index) const { return RawRefBoneInfo.IsValidIndex(Index); }
    // The number of bones from a bone up to its ancestor; INDEX_NONE when it is no ancestor.
    int32 GetDepthBetweenBones(int32 BoneIndex, int32 ParentBoneIndex) const;
    bool BoneIsChildOf(int32 ChildBoneIndex, int32 ParentBoneIndex) const;
    // The bones whose parent is the bone; returns their count.
    int32 GetDirectChildBones(int32 ParentBoneIndex, TArray<int32>& Children) const;
    // Adds the missing parents of the bones of a sorted array, then sorts it (UE's EnsureParentsExistAndSort).
    void EnsureParentsExist(TArray<FBoneIndexType>& InOutBoneSortedArray) const;
    void EnsureParentsExistAndSort(TArray<FBoneIndexType>& InOutBoneUnsortedArray) const;

    friend FArchive& operator<<(FArchive& Ar, FReferenceSkeleton& F);

private:
    void RebuildNameToIndexMap();
    int32 GetRawSourceBoneIndex(const USkeleton* Skeleton, FName SourceBoneName) const;

    TArray<FMeshBoneInfo> RawRefBoneInfo;
    TArray<FTransform> RawRefBonePose;
    TArray<FMeshBoneInfo> FinalRefBoneInfo;
    TArray<FTransform> FinalRefBonePose;
    TMap<FName, int32> RawNameToIndexMap;
    TMap<FName, int32> FinalNameToIndexMap;
    TArray<FBoneIndexType> RequiredVirtualBones;
    TArray<FVirtualBoneRefData> UsedVirtualBoneData;
};

}
