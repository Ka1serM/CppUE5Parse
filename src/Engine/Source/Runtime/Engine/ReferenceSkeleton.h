#pragma once

#include "Engine/Source/Runtime/AnimationCore/BoneIndices.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

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

struct FVirtualBoneRefData {
    int32 VBRefSkelIndex = INDEX_NONE;
    int32 SourceRefSkelIndex = INDEX_NONE;
    int32 TargetRefSkelIndex = INDEX_NONE;
};

struct FReferenceSkeleton {
public:
    void RebuildRefSkeleton(const USkeleton* Skeleton, bool bRebuildNameMap);

    int32 GetNum() const { return FinalRefBoneInfo.Num(); }
    int32 GetRawBoneNum() const { return RawRefBoneInfo.Num(); }
    const TArray<FVirtualBoneRefData>& GetVirtualBoneRefData() const { return UsedVirtualBoneData; }
    const TArray<FMeshBoneInfo>& GetRefBoneInfo() const { return FinalRefBoneInfo; }
    const TArray<FTransform>& GetRefBonePose() const { return FinalRefBonePose; }
    const TArray<FTransform>& GetRawRefBonePose() const { return RawRefBonePose; }
    void Empty();

    int32 FindBoneIndex(FName BoneName) const;
    FName GetBoneName(int32 BoneIndex) const { return FinalRefBoneInfo[BoneIndex].Name; }
    int32 GetParentIndex(int32 BoneIndex) const { return FinalRefBoneInfo[BoneIndex].ParentIndex; }
    int32 GetRawParentIndex(int32 BoneIndex) const { return RawRefBoneInfo[BoneIndex].ParentIndex; }
    bool IsValidIndex(int32 Index) const { return FinalRefBoneInfo.IsValidIndex(Index); }
    int32 GetDepthBetweenBones(int32 BoneIndex, int32 ParentBoneIndex) const;
    bool BoneIsChildOf(int32 ChildBoneIndex, int32 ParentBoneIndex) const;

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
