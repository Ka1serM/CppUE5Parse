#include "Engine/Source/Runtime/Engine/ReferenceSkeleton.h"

#include <algorithm>

#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"

namespace ue5parse::runtime {

namespace {

FTransform GetComponentSpaceTransform(TArray<uint8>& ComponentSpaceFlags, TArray<FTransform>& ComponentSpaceTransforms, const FReferenceSkeleton& RefSkeleton,
                                      int32 TargetIndex) {
    FTransform& This = ComponentSpaceTransforms[TargetIndex];
    if (!ComponentSpaceFlags[TargetIndex]) {
        const int32 ParentIndex = RefSkeleton.GetRawParentIndex(TargetIndex);
        This *= GetComponentSpaceTransform(ComponentSpaceFlags, ComponentSpaceTransforms, RefSkeleton, ParentIndex);
        ComponentSpaceFlags[TargetIndex] = 1;
    }
    return This;
}

}

FArchive& operator<<(FArchive& Ar, FReferenceSkeleton& F) {
    Ar << F.RawRefBoneInfo << F.RawRefBonePose;
    const int32 MapCount = Ar.ReadCount(12);
    F.RawNameToIndexMap.Empty();
    for (int32 i = 0; i < MapCount && !Ar.IsError(); i++) {
        FName Name;
        int32 Index = 0;
        Ar << Name << Index;
        F.RawNameToIndexMap.Add(Name, Index);
    }
    F.FinalRefBoneInfo = F.RawRefBoneInfo;
    F.FinalRefBonePose = F.RawRefBonePose;
    F.FinalNameToIndexMap = F.RawNameToIndexMap;
    return Ar;
}

void FReferenceSkeleton::Empty() {
    RawRefBoneInfo.Empty();
    RawRefBonePose.Empty();
    FinalRefBoneInfo.Empty();
    FinalRefBonePose.Empty();
    RawNameToIndexMap.Empty();
    FinalNameToIndexMap.Empty();
    RequiredVirtualBones.Empty();
    UsedVirtualBoneData.Empty();
}

void FReferenceSkeleton::RebuildNameToIndexMap() {
    RawNameToIndexMap.Empty();
    for (int32 BoneIndex = 0; BoneIndex < RawRefBoneInfo.Num(); BoneIndex++)
        RawNameToIndexMap.Add(RawRefBoneInfo[BoneIndex].Name, BoneIndex);
}

int32 FReferenceSkeleton::GetRawSourceBoneIndex(const USkeleton* Skeleton, FName SourceBoneName) const {
    for (const FVirtualBone& VB : Skeleton->GetVirtualBones())
        if (VB.VirtualBoneName == SourceBoneName)
            return FindBoneIndex(VB.TargetBoneName);
    return FindBoneIndex(SourceBoneName);
}

void FReferenceSkeleton::RebuildRefSkeleton(const USkeleton* Skeleton, bool bRebuildNameMap) {
    if (bRebuildNameMap)
        RebuildNameToIndexMap();
    const int32 NumVirtualBones = Skeleton ? Skeleton->GetVirtualBones().Num() : 0;
    FinalRefBoneInfo = RawRefBoneInfo;
    FinalRefBonePose = RawRefBonePose;
    FinalNameToIndexMap = RawNameToIndexMap;
    RequiredVirtualBones.Reset();
    UsedVirtualBoneData.Reset();
    if (NumVirtualBones == 0 || RawRefBonePose.Num() == 0)
        return;
    TArray<uint8> ComponentSpaceFlags;
    ComponentSpaceFlags.SetNum(RawRefBonePose.Num());
    ComponentSpaceFlags[0] = 1;
    TArray<FTransform> ComponentSpaceTransforms = RawRefBonePose;
    for (int32 VirtualBoneIdx = 0; VirtualBoneIdx < NumVirtualBones; ++VirtualBoneIdx) {
        const FVirtualBone& VB = Skeleton->GetVirtualBones()[VirtualBoneIdx];
        const int32 SourceIndex = GetRawSourceBoneIndex(Skeleton, VB.SourceBoneName);
        const int32 ParentIndex = FindBoneIndex(VB.SourceBoneName);
        const int32 TargetIndex = FindBoneIndex(VB.TargetBoneName);
        if (ParentIndex == INDEX_NONE || TargetIndex == INDEX_NONE || SourceIndex == INDEX_NONE)
            continue;
        FinalRefBoneInfo.Add(FMeshBoneInfo(VB.VirtualBoneName, ParentIndex));
        const FTransform TargetCS = GetComponentSpaceTransform(ComponentSpaceFlags, ComponentSpaceTransforms, *this, TargetIndex);
        const FTransform SourceCS = GetComponentSpaceTransform(ComponentSpaceFlags, ComponentSpaceTransforms, *this, SourceIndex);
        const int32 NewBoneIndex = FinalRefBonePose.Add(TargetCS.GetRelativeTransform(SourceCS));
        FinalNameToIndexMap.Add(VB.VirtualBoneName, NewBoneIndex);
        RequiredVirtualBones.Add(static_cast<FBoneIndexType>(NewBoneIndex));
        UsedVirtualBoneData.Add(FVirtualBoneRefData{NewBoneIndex, SourceIndex, TargetIndex});
    }
}

int32 FReferenceSkeleton::FindBoneIndex(FName BoneName) const {
    if (BoneName.isNone())
        return INDEX_NONE;
    const int32* Index = FinalNameToIndexMap.Find(BoneName);
    return Index ? *Index : INDEX_NONE;
}

int32 FReferenceSkeleton::GetDepthBetweenBones(int32 BoneIndex, int32 ParentBoneIndex) const {
    if (BoneIndex < ParentBoneIndex)
        return INDEX_NONE;
    int32 CurBoneIndex = BoneIndex;
    int32 Depth = 0;
    do {
        if (CurBoneIndex == ParentBoneIndex)
            return Depth;
        CurBoneIndex = FinalRefBoneInfo[CurBoneIndex].ParentIndex;
        ++Depth;
    } while (CurBoneIndex != INDEX_NONE);
    return INDEX_NONE;
}

bool FReferenceSkeleton::BoneIsChildOf(int32 ChildBoneIndex, int32 ParentBoneIndex) const {
    if (ParentBoneIndex == INDEX_NONE || ChildBoneIndex <= ParentBoneIndex)
        return false;
    for (int32 BoneIndex = GetParentIndex(ChildBoneIndex); BoneIndex != INDEX_NONE; BoneIndex = GetParentIndex(BoneIndex))
        if (BoneIndex == ParentBoneIndex)
            return true;
    return false;
}

}
