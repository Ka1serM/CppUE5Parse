#pragma once

#include <memory>

#include "ue5parse/runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "ue5parse/runtime/Engine/ReferenceSkeleton.h"

#include "Skeleton.generated.h"

namespace ue5parse::runtime {

class USkinnedAsset;
class UBlendProfile;

namespace EBoneTranslationRetargetingMode {
enum Type : int32 { Animation, Skeleton, AnimationScaled, AnimationRelative, OrientAndScale };
}

// FBoneNode: the retargeting mode of a skeleton bone.
USTRUCT()
struct FBoneNode {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<EBoneTranslationRetargetingMode::Type> TranslationRetargetingMode = EBoneTranslationRetargetingMode::Animation;
};

// FVirtualBone: a bone between two bones of the skeleton, added to the final bones of its meshes.
USTRUCT()
struct FVirtualBone {
    GENERATED_BODY()
    UPROPERTY()
    FName SourceBoneName;
    UPROPERTY()
    FName TargetBoneName;
    UPROPERTY()
    FName VirtualBoneName;
};

// FAnimSlotGroup: montage slots that share blending.
USTRUCT()
struct FAnimSlotGroup {
    GENERATED_BODY()
    UPROPERTY()
    FName GroupName;
    UPROPERTY()
    TArray<FName> SlotNames;

    static const FName DefaultGroupName;
    static const FName DefaultSlotName;
};

// FReferencePose: a named reference pose of another mesh (a retarget source).
USTRUCT()
struct FReferencePose {
    GENERATED_BODY()
    UPROPERTY()
    FName PoseName;
    UPROPERTY()
    TArray<FTransform> ReferencePose;
};

// USkeleton (UE's Animation/Skeleton.h): the bone hierarchy that animations are made for.
UCLASS()
class USkeleton : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FBoneNode> BoneTree;
    UPROPERTY()
    FGuid VirtualBoneGuid;
    UPROPERTY()
    TArray<FVirtualBone> VirtualBones;
    UPROPERTY()
    TArray<TObjectPtr<USkeletalMeshSocket>> Sockets;
    UPROPERTY()
    TArray<TObjectPtr<UBlendProfile>> BlendProfiles;
    UPROPERTY()
    TArray<FAnimSlotGroup> SlotGroups;

    void Serialize(FArchive& Ar) override;
    const FReferenceSkeleton& GetReferenceSkeleton() const { return ReferenceSkeleton; }
    const TArray<FVirtualBone>& GetVirtualBones() const { return VirtualBones; }
    EBoneTranslationRetargetingMode::Type GetBoneTranslationRetargetingMode(int32 BoneTreeIdx, bool bDisableRetargeting) const {
        return bDisableRetargeting ? EBoneTranslationRetargetingMode::Animation : GetBoneTranslationRetargetingMode(BoneTreeIdx);
    }
    EBoneTranslationRetargetingMode::Type GetBoneTranslationRetargetingMode(int32 BoneTreeIdx) const {
        return BoneTree.IsValidIndex(BoneTreeIdx) ? BoneTree[BoneTreeIdx].TranslationRetargetingMode.GetValue()
                                                  : EBoneTranslationRetargetingMode::Animation;
    }
    const TMap<FName, FReferencePose>& GetAnimRetargetSources() const { return AnimRetargetSources; }
    // The reference pose of a retarget source; the skeleton's own when it has no source of that name (UE's
    // GetRefLocalPoses).
    const TArray<FTransform>& GetRefLocalPoses(FName RetargetSource = FName()) const;
    const FGuid& GetGuid() const { return Guid; }
    const FGuid& GetVirtualBoneGuid() const { return VirtualBoneGuid; }
    USkeletalMeshSocket* FindSocket(FName InSocketName) const;
    USkeletalMeshSocket* FindSocketAndIndex(FName InSocketName, int32& OutIndex) const;
    // The group of a slot; None when no group has it (UE's GetSlotGroupName).
    FName GetSlotGroupName(FName SlotName) const;
    const FAnimSlotGroup* FindAnimSlotGroup(FName InGroupName) const;
    bool ContainsSlotName(FName InSlotName) const { return SlotToGroupNameMap.Contains(InSlotName); }
    // The bone of the skeleton for a bone of a mesh (by name); INDEX_NONE when the skeleton has none.
    int32 GetSkeletonBoneIndexFromMeshBoneIndex(const USkinnedAsset* InSkinnedAsset, int32 MeshBoneIndex) const;
    int32 GetMeshBoneIndexFromSkeletonBoneIndex(const USkinnedAsset* InSkinnedAsset, int32 SkeletonBoneIndex) const;
    // The index of a mesh's bones for each skeleton bone (and back), made on first use for each mesh (UE's
    // FSkeletonToMeshLinkup).
    const TArray<int32>& FindOrAddMeshToSkeletonLinkup(const USkinnedAsset* InSkinnedAsset) const;
    const TArray<int32>& FindOrAddSkeletonToMeshLinkup(const USkinnedAsset* InSkinnedAsset) const;
    UFUNCTION(BlueprintCallable)
    UBlendProfile* GetBlendProfile(const FName& InProfileName);

private:
    void BuildSlotToGroupMap();
    void SerializeSmartNames(FArchive& Ar);

    FReferenceSkeleton ReferenceSkeleton;
    TMap<FName, FReferencePose> AnimRetargetSources;
    FGuid Guid;
    TMap<FName, FName> SlotToGroupNameMap;
    struct FSkeletonToMeshLinkup {
        TArray<int32> SkeletonToMeshTable;
        TArray<int32> MeshToSkeletonTable;
    };
    mutable TMap<const USkinnedAsset*, std::shared_ptr<FSkeletonToMeshLinkup>> LinkupCache;
};

}
