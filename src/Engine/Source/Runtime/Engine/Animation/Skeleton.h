#pragma once

#include <memory>

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "Engine/Source/Runtime/Engine/ReferenceSkeleton.h"

#include "Skeleton.generated.h"

namespace ue5parse::runtime {

class USkinnedAsset;
class UBlendProfile;

namespace EBoneTranslationRetargetingMode {

enum Type : int32 { Animation, Skeleton, AnimationScaled, AnimationRelative, OrientAndScale };

}

USTRUCT()
struct FBoneNode {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<EBoneTranslationRetargetingMode::Type> TranslationRetargetingMode = EBoneTranslationRetargetingMode::Animation;
};

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

USTRUCT()
struct FReferencePose {
    GENERATED_BODY()
    UPROPERTY()
    FName PoseName;
    UPROPERTY()
    TArray<FTransform> ReferencePose;
};

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
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;

    void Serialize(FArchive& Ar) override;
    const FReferenceSkeleton& GetReferenceSkeleton() const { return ReferenceSkeleton; }
    const TArray<FVirtualBone>& GetVirtualBones() const { return VirtualBones; }
    EBoneTranslationRetargetingMode::Type GetBoneTranslationRetargetingMode(int32 BoneTreeIdx, bool bDisableRetargeting) const {
        return bDisableRetargeting ? EBoneTranslationRetargetingMode::Animation : GetBoneTranslationRetargetingMode(BoneTreeIdx);
    }
    EBoneTranslationRetargetingMode::Type GetBoneTranslationRetargetingMode(int32 BoneTreeIdx) const {
        return BoneTree.IsValidIndex(BoneTreeIdx) ? BoneTree[BoneTreeIdx].TranslationRetargetingMode.GetValue() : EBoneTranslationRetargetingMode::Animation;
    }
    const TArray<FTransform>& GetRefLocalPoses(FName RetargetSource = FName()) const;
    const FGuid& GetGuid() const { return Guid; }
    const FGuid& GetVirtualBoneGuid() const { return VirtualBoneGuid; }
    USkeletalMeshSocket* FindSocket(FName InSocketName) const;
    USkeletalMeshSocket* FindSocketAndIndex(FName InSocketName, int32& OutIndex) const;
    FName GetSlotGroupName(FName SlotName) const;
    const TArray<int32>& FindOrAddMeshToSkeletonLinkup(const USkinnedAsset* InSkinnedAsset) const;
    const TArray<int32>& FindOrAddSkeletonToMeshLinkup(const USkinnedAsset* InSkinnedAsset) const;
    bool IsCompatibleMesh(const USkinnedAsset* InSkinnedAsset) const;
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
