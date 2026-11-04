#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"

#include <mutex>

#include "Engine/Source/Runtime/Engine/Animation/BlendProfile.h"
#include "Engine/Source/Runtime/Engine/Engine/SkinnedAsset.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

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
    const int32 MappingCount = Ar.ReadCount(12);
    for (int32 i = 0; i < MappingCount && !Ar.IsError(); i++) {
        FName MappingName;
        Ar << MappingName;
        const int32 CurveCount = Ar.ReadCount(8 + 8 + 4 + 1);
        for (int32 c = 0; c < CurveCount && !Ar.IsError(); c++) {
            FName CurveName;
            bool bMaterial = false, bMorphtarget = false;
            Ar << CurveName << bMaterial << bMorphtarget;
            Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(8)) * 8);
            Ar.SkipBytes(1);
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

const TArray<int32>& USkeleton::FindOrAddSkeletonToMeshLinkup(const USkinnedAsset* InSkinnedAsset) const {
    std::lock_guard Lock(GetLinkupMutex());
    if (const std::shared_ptr<FSkeletonToMeshLinkup>* Found = LinkupCache.Find(InSkinnedAsset)) return (*Found)->SkeletonToMeshTable;
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

bool USkeleton::IsCompatibleMesh(const USkinnedAsset* InSkinnedAsset) const {
    const FReferenceSkeleton& MeshRefSkeleton = InSkinnedAsset->GetRefSkeleton();
    int32 NumOfBoneMatches = 0;
    for (int32 MeshBoneIndex = 0; MeshBoneIndex < MeshRefSkeleton.GetRawBoneNum(); ++MeshBoneIndex) {
        int32 MatchedBoneIndex = MeshBoneIndex;
        while (MatchedBoneIndex != INDEX_NONE && ReferenceSkeleton.FindBoneIndex(MeshRefSkeleton.GetBoneName(MatchedBoneIndex)) == INDEX_NONE)
            MatchedBoneIndex = MeshRefSkeleton.GetParentIndex(MatchedBoneIndex);
        if (MatchedBoneIndex == INDEX_NONE) return false;
        if (MatchedBoneIndex == MeshBoneIndex) ++NumOfBoneMatches;
    }
    return NumOfBoneMatches > 0;
}

UBlendProfile* USkeleton::GetBlendProfile(const FName& InProfileName) {
    for (UBlendProfile* Profile : BlendProfiles)
        if (Profile != nullptr && Profile->GetFName() == InProfileName) return Profile;
    return nullptr;
}

}
