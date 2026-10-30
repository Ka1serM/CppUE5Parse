#include "ue5parse/runtime/Engine/Animation/Skeleton.h"

#include <mutex>

#include "ue5parse/runtime/Engine/Animation/BlendProfile.h"
#include "ue5parse/runtime/Engine/Engine/SkinnedAsset.h"
#include "ue5parse/runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

const FName FAnimSlotGroup::DefaultGroupName("DefaultGroup");
const FName FAnimSlotGroup::DefaultSlotName("DefaultSlot");

namespace {

std::mutex& GetLinkupMutex() {
    static std::mutex Mutex;
    return Mutex;
}

}

void USkeleton::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    Ar << ReferenceSkeleton;
    const int32 NumOfRetargetSources = Ar.ReadCount(12);
    AnimRetargetSources.Empty();
    for (int32 Index = 0; Index < NumOfRetargetSources && !Ar.IsError(); ++Index) {
        FName RetargetSourceName;
        FReferencePose RetargetSource;
        Ar << RetargetSourceName << RetargetSource.PoseName << RetargetSource.ReferencePose;
        AnimRetargetSources.Add(RetargetSourceName, RetargetSource);
    }
    Ar << Guid;
    SerializeSmartNames(Ar);
    BuildSlotToGroupMap();
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsEditorDataStripped()) {
        TArray<FName> ExistingMarkerNames;
        Ar << ExistingMarkerNames;
    }
    ReferenceSkeleton.RebuildRefSkeleton(this, false);
}

void USkeleton::SerializeSmartNames(FArchive& Ar) {
    // The deprecated smart name container: each container's curve names and their metadata.
    const int32 MappingCount = Ar.ReadCount(12);
    for (int32 i = 0; i < MappingCount && !Ar.IsError(); i++) {
        FName MappingName;
        Ar << MappingName;
        const int32 CurveCount = Ar.ReadCount(8 + 8 + 4 + 1);
        for (int32 c = 0; c < CurveCount && !Ar.IsError(); c++) {
            FName CurveName;
            bool bMaterial = false, bMorphtarget = false;
            Ar << CurveName << bMaterial << bMorphtarget;
            Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(8)) * 8);   // LinkedBones
            Ar.SkipBytes(1);                                         // MaxLOD
        }
    }
}

void USkeleton::BuildSlotToGroupMap() {
    SlotToGroupNameMap.Empty();
    for (const FAnimSlotGroup& SlotGroup : SlotGroups)
        for (const FName& SlotName : SlotGroup.SlotNames) SlotToGroupNameMap.Add(SlotName, SlotGroup.GroupName);
}

const TArray<FTransform>& USkeleton::GetRefLocalPoses(FName RetargetSource) const {
    if (!RetargetSource.isNone())
        if (const FReferencePose* FoundRetargetSource = AnimRetargetSources.Find(RetargetSource)) return FoundRetargetSource->ReferencePose;
    return ReferenceSkeleton.GetRefBonePose();
}

USkeletalMeshSocket* USkeleton::FindSocket(FName InSocketName) const {
    int32 DummyIndex;
    return FindSocketAndIndex(InSocketName, DummyIndex);
}

USkeletalMeshSocket* USkeleton::FindSocketAndIndex(FName InSocketName, int32& OutIndex) const {
    OutIndex = INDEX_NONE;
    if (InSocketName.isNone()) return nullptr;
    for (int32 i = 0; i < Sockets.Num(); i++) {
        if (Sockets[i] != nullptr && Sockets[i]->SocketName == InSocketName) {
            OutIndex = i;
            return Sockets[i];
        }
    }
    return nullptr;
}

FName USkeleton::GetSlotGroupName(FName SlotName) const {
    const FName* Found = SlotToGroupNameMap.Find(SlotName);
    return Found ? *Found : FName();
}

const FAnimSlotGroup* USkeleton::FindAnimSlotGroup(FName InGroupName) const {
    for (const FAnimSlotGroup& Group : SlotGroups)
        if (Group.GroupName == InGroupName) return &Group;
    return nullptr;
}

const TArray<int32>& USkeleton::FindOrAddSkeletonToMeshLinkup(const USkinnedAsset* InSkinnedAsset) const {
    std::lock_guard Lock(GetLinkupMutex());
    if (const std::shared_ptr<FSkeletonToMeshLinkup>* Found = LinkupCache.Find(InSkinnedAsset)) return (*Found)->SkeletonToMeshTable;
    // Bones match by name; a bone of one that the other does not have maps to INDEX_NONE.
    FSkeletonToMeshLinkup& Linkup = *LinkupCache.Add(InSkinnedAsset, std::make_shared<FSkeletonToMeshLinkup>());
    const FReferenceSkeleton& MeshRefSkeleton = InSkinnedAsset->GetRefSkeleton();
    Linkup.SkeletonToMeshTable.Empty(ReferenceSkeleton.GetNum());
    for (int32 SkeletonBoneIndex = 0; SkeletonBoneIndex < ReferenceSkeleton.GetNum(); SkeletonBoneIndex++)
        Linkup.SkeletonToMeshTable.Add(MeshRefSkeleton.FindBoneIndex(ReferenceSkeleton.GetBoneName(SkeletonBoneIndex)));
    Linkup.MeshToSkeletonTable.Empty(MeshRefSkeleton.GetNum());
    for (int32 MeshBoneIndex = 0; MeshBoneIndex < MeshRefSkeleton.GetNum(); MeshBoneIndex++)
        Linkup.MeshToSkeletonTable.Add(ReferenceSkeleton.FindBoneIndex(MeshRefSkeleton.GetBoneName(MeshBoneIndex)));
    return Linkup.SkeletonToMeshTable;
}

const TArray<int32>& USkeleton::FindOrAddMeshToSkeletonLinkup(const USkinnedAsset* InSkinnedAsset) const {
    FindOrAddSkeletonToMeshLinkup(InSkinnedAsset);
    std::lock_guard Lock(GetLinkupMutex());
    return LinkupCache.FindChecked(InSkinnedAsset)->MeshToSkeletonTable;
}

int32 USkeleton::GetSkeletonBoneIndexFromMeshBoneIndex(const USkinnedAsset* InSkinnedAsset, int32 MeshBoneIndex) const {
    const TArray<int32>& Table = FindOrAddMeshToSkeletonLinkup(InSkinnedAsset);
    return Table.IsValidIndex(MeshBoneIndex) ? Table[MeshBoneIndex] : INDEX_NONE;
}

int32 USkeleton::GetMeshBoneIndexFromSkeletonBoneIndex(const USkinnedAsset* InSkinnedAsset, int32 SkeletonBoneIndex) const {
    const TArray<int32>& Table = FindOrAddSkeletonToMeshLinkup(InSkinnedAsset);
    return Table.IsValidIndex(SkeletonBoneIndex) ? Table[SkeletonBoneIndex] : INDEX_NONE;
}

UBlendProfile* USkeleton::GetBlendProfile(const FName& InProfileName) {
    for (UBlendProfile* Profile : BlendProfiles)
        if (Profile != nullptr && Profile->GetFName() == InProfileName) return Profile;
    return nullptr;
}

}
