#include "ue5parse/runtime/Engine/ReferenceSkeleton.h"

#include <algorithm>

#include "ue5parse/runtime/Engine/Animation/Skeleton.h"

namespace ue5parse::runtime {

namespace {

// The component space transform of a raw bone, computed on demand from its parents.
FTransform GetComponentSpaceTransform(TArray<uint8>& ComponentSpaceFlags, TArray<FTransform>& ComponentSpaceTransforms,
                                      const FReferenceSkeleton& RefSkeleton, int32 TargetIndex) {
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
    for (int32 BoneIndex = 0; BoneIndex < RawRefBoneInfo.Num(); BoneIndex++) RawNameToIndexMap.Add(RawRefBoneInfo[BoneIndex].Name, BoneIndex);
}

int32 FReferenceSkeleton::GetRawSourceBoneIndex(const USkeleton* Skeleton, FName SourceBoneName) const {
    // A virtual bone made from another virtual bone uses that one's target, which is a raw bone.
    for (const FVirtualBone& VB : Skeleton->GetVirtualBones())
        if (VB.VirtualBoneName == SourceBoneName) return FindBoneIndex(VB.TargetBoneName);
    return FindBoneIndex(SourceBoneName);
}

void FReferenceSkeleton::RebuildRefSkeleton(const USkeleton* Skeleton, bool bRebuildNameMap) {
    if (bRebuildNameMap) RebuildNameToIndexMap();
    const int32 NumVirtualBones = Skeleton ? Skeleton->GetVirtualBones().Num() : 0;
    FinalRefBoneInfo = RawRefBoneInfo;
    FinalRefBonePose = RawRefBonePose;
    FinalNameToIndexMap = RawNameToIndexMap;
    RequiredVirtualBones.Reset();
    UsedVirtualBoneData.Reset();
    if (NumVirtualBones == 0 || RawRefBonePose.Num() == 0) return;
    TArray<uint8> ComponentSpaceFlags;
    ComponentSpaceFlags.SetNum(RawRefBonePose.Num());
    ComponentSpaceFlags[0] = 1;
    TArray<FTransform> ComponentSpaceTransforms = RawRefBonePose;
    for (int32 VirtualBoneIdx = 0; VirtualBoneIdx < NumVirtualBones; ++VirtualBoneIdx) {
        const FVirtualBone& VB = Skeleton->GetVirtualBones()[VirtualBoneIdx];
        const int32 SourceIndex = GetRawSourceBoneIndex(Skeleton, VB.SourceBoneName);
        const int32 ParentIndex = FindBoneIndex(VB.SourceBoneName);
        const int32 TargetIndex = FindBoneIndex(VB.TargetBoneName);
        if (ParentIndex == INDEX_NONE || TargetIndex == INDEX_NONE || SourceIndex == INDEX_NONE) continue;
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
    if (BoneName.isNone()) return INDEX_NONE;
    const int32* Index = FinalNameToIndexMap.Find(BoneName);
    return Index ? *Index : INDEX_NONE;
}

int32 FReferenceSkeleton::FindRawBoneIndex(FName BoneName) const {
    if (BoneName.isNone()) return INDEX_NONE;
    const int32* Index = RawNameToIndexMap.Find(BoneName);
    return Index ? *Index : INDEX_NONE;
}

int32 FReferenceSkeleton::GetDepthBetweenBones(int32 BoneIndex, int32 ParentBoneIndex) const {
    if (BoneIndex < ParentBoneIndex) return INDEX_NONE;
    int32 CurBoneIndex = BoneIndex;
    int32 Depth = 0;
    do {
        if (CurBoneIndex == ParentBoneIndex) return Depth;
        CurBoneIndex = FinalRefBoneInfo[CurBoneIndex].ParentIndex;
        ++Depth;
    } while (CurBoneIndex != INDEX_NONE);
    return INDEX_NONE;
}

bool FReferenceSkeleton::BoneIsChildOf(int32 ChildBoneIndex, int32 ParentBoneIndex) const {
    if (ParentBoneIndex == INDEX_NONE || ChildBoneIndex <= ParentBoneIndex) return false;
    for (int32 BoneIndex = GetParentIndex(ChildBoneIndex); BoneIndex != INDEX_NONE; BoneIndex = GetParentIndex(BoneIndex))
        if (BoneIndex == ParentBoneIndex) return true;
    return false;
}

int32 FReferenceSkeleton::GetDirectChildBones(int32 ParentBoneIndex, TArray<int32>& Children) const {
    Children.Reset();
    for (int32 ChildIndex = ParentBoneIndex + 1; ChildIndex < GetNum(); ChildIndex++)
        if (GetParentIndex(ChildIndex) == ParentBoneIndex) Children.Add(ChildIndex);
    return Children.Num();
}

void FReferenceSkeleton::EnsureParentsExist(TArray<FBoneIndexType>& InOutBoneSortedArray) const {
    const int32 NumBones = GetNum();
    TArray<uint8> BoneExists;
    BoneExists.SetNum(NumBones);
    int32 i = 0;
    while (i < InOutBoneSortedArray.Num()) {
        const int32 BoneIndex = InOutBoneSortedArray[i];
        if (BoneIndex <= 0) {
            if (NumBones > 0) BoneExists[0] = 1;
            i++;
            continue;
        }
        if (BoneIndex >= NumBones) {
            i++;
            continue;
        }
        BoneExists[BoneIndex] = 1;
        const int32 ParentIndex = GetParentIndex(BoneIndex);
        // A missing parent goes in at this place; i stays, so that its own parent is checked next.
        if (!BoneExists[ParentIndex]) {
            InOutBoneSortedArray.Insert(static_cast<FBoneIndexType>(ParentIndex), i);
            BoneExists[ParentIndex] = 1;
        } else {
            i++;
        }
    }
}

void FReferenceSkeleton::EnsureParentsExistAndSort(TArray<FBoneIndexType>& InOutBoneUnsortedArray) const {
    std::sort(InOutBoneUnsortedArray.begin(), InOutBoneUnsortedArray.end());
    EnsureParentsExist(InOutBoneUnsortedArray);
    std::sort(InOutBoneUnsortedArray.begin(), InOutBoneUnsortedArray.end());
}

}
