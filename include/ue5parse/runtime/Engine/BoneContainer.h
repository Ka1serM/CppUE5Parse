#pragma once

#include "ue5parse/runtime/Engine/ReferenceSkeleton.h"

// The bones that a pose evaluates (UE's BoneContainer.h): the required bones of a mesh LOD or a skeleton in compact
// order, their parents, and the maps between compact, mesh and skeleton indices.

namespace ue5parse::runtime {

class UObject;
class USkeleton;
class USkeletalMesh;

struct FVirtualBoneCompactPoseData {
    FCompactPoseBoneIndex VBIndex;
    FCompactPoseBoneIndex SourceIndex;
    FCompactPoseBoneIndex TargetIndex;
};

// The cached data of the OrientAndScale translation retargeting of one bone.
struct FOrientAndScaleRetargetingCachedData {
    FQuat TranslationDeltaOrient = FQuat::Identity;
    float TranslationScale = 1;
    FVector SourceTranslation = FVector::ZeroVector;
    FVector TargetTranslation = FVector::ZeroVector;
};

struct FRetargetSourceCachedData {
    TArray<FOrientAndScaleRetargetingCachedData> OrientAndScaleData;
    TArray<int32> CompactPoseIndexToOrientAndScaleIndex;
};

struct FBoneContainer {
    FBoneContainer() = default;
    FBoneContainer(const TArray<FBoneIndexType>& InRequiredBoneIndexArray, UObject& InAsset);
    void InitializeTo(const TArray<FBoneIndexType>& InRequiredBoneIndexArray, UObject& InAsset);
    void Reset();

    bool IsValid() const { return Asset != nullptr && RefSkeleton != nullptr && BoneIndicesArray.Num() > 0; }
    UObject* GetAsset() const { return Asset; }
    USkeletalMesh* GetSkeletalMeshAsset() const { return AssetSkeletalMesh; }
    USkeleton* GetSkeletonAsset() const { return AssetSkeleton; }
    void SetDisableRetargeting(bool bInDisableRetargeting) { bDisableRetargeting = bInDisableRetargeting; }
    bool GetDisableRetargeting() const { return bDisableRetargeting; }
    // Changes each time the container is initialized, so that a node can tell when its cached bone data is stale.
    uint16 GetSerialNumber() const { return SerialNumber; }

    // The mesh pose index of each compact pose bone, parents first.
    const TArray<FBoneIndexType>& GetBoneIndicesArray() const { return BoneIndicesArray; }
    const TArray<FTransform>& GetRefPoseArray() const { return RefSkeleton->GetRefBonePose(); }
    const TArray<FCompactPoseBoneIndex>& GetCompactPoseParentBoneArray() const { return CompactPoseParentBones; }
    const FTransform& GetRefPoseTransform(const FCompactPoseBoneIndex& BoneIndex) const {
        return RefSkeleton->GetRefBonePose()[BoneIndicesArray[BoneIndex.GetInt()]];
    }
    void FillWithCompactRefPose(TArray<FTransform>& OutTransforms) const;
    const FReferenceSkeleton& GetReferenceSkeleton() const { return *RefSkeleton; }
    int32 GetNumBones() const { return RefSkeleton->GetNum(); }
    int32 GetCompactPoseNumBones() const { return BoneIndicesArray.Num(); }
    const TArray<FVirtualBoneCompactPoseData>& GetVirtualBoneCompactPoseData() const { return VirtualBoneCompactPoseData; }

    // The mesh pose index of a bone name; INDEX_NONE when the mesh does not have it.
    int32 GetPoseBoneIndexForBoneName(const FName& BoneName) const { return RefSkeleton->FindBoneIndex(BoneName); }
    int32 GetParentBoneIndex(int32 BoneIndex) const { return RefSkeleton->GetParentIndex(BoneIndex); }
    FCompactPoseBoneIndex GetParentBoneIndex(const FCompactPoseBoneIndex& BoneIndex) const { return CompactPoseParentBones[BoneIndex.GetInt()]; }
    int32 GetDepthBetweenBones(int32 BoneIndex, int32 ParentBoneIndex) const { return RefSkeleton->GetDepthBetweenBones(BoneIndex, ParentBoneIndex); }
    bool BoneIsChildOf(int32 BoneIndex, int32 ParentBoneIndex) const { return RefSkeleton->BoneIsChildOf(BoneIndex, ParentBoneIndex); }
    bool BoneIsChildOf(const FCompactPoseBoneIndex& BoneIndex, const FCompactPoseBoneIndex& ParentBoneIndex) const;
    bool Contains(FBoneIndexType NewIndex) const { return NewIndex < BoneSwitchArray.Num() && BoneSwitchArray[NewIndex]; }

    // The skeleton bone of each mesh pose bone, and back.
    const TArray<int32>& GetSkeletonToPoseBoneIndexArray() const { return SkeletonToPoseBoneIndexArray; }
    const TArray<int32>& GetPoseToSkeletonBoneIndexArray() const { return PoseToSkeletonBoneIndexArray; }
    int32 GetSkeletonIndex(const FCompactPoseBoneIndex& BoneIndex) const { return CompactPoseToSkeletonIndex[BoneIndex.GetInt()]; }
    FCompactPoseBoneIndex GetCompactPoseIndexFromSkeletonIndex(int32 SkeletonIndex) const {
        return SkeletonToCompactPose.IsValidIndex(SkeletonIndex) ? SkeletonToCompactPose[SkeletonIndex] : FCompactPoseBoneIndex(INDEX_NONE);
    }
    FSkeletonPoseBoneIndex GetSkeletonPoseIndexFromCompactPoseIndex(const FCompactPoseBoneIndex& BoneIndex) const {
        return FSkeletonPoseBoneIndex(CompactPoseToSkeletonIndex[BoneIndex.GetInt()]);
    }
    FMeshPoseBoneIndex MakeMeshPoseIndex(const FCompactPoseBoneIndex& BoneIndex) const {
        return FMeshPoseBoneIndex(BoneIndicesArray[BoneIndex.GetInt()]);
    }
    FCompactPoseBoneIndex MakeCompactPoseIndex(const FMeshPoseBoneIndex& BoneIndex) const;
    FMeshPoseBoneIndex GetMeshPoseIndexFromSkeletonPoseIndex(const FSkeletonPoseBoneIndex& BoneIndex) const {
        return FMeshPoseBoneIndex(SkeletonToPoseBoneIndexArray.IsValidIndex(BoneIndex.GetInt()) ? SkeletonToPoseBoneIndexArray[BoneIndex.GetInt()]
                                                                                               : INDEX_NONE);
    }

    // The OrientAndScale data of a retarget source, made on first use (UE's GetRetargetSourceCachedData).
    const FRetargetSourceCachedData& GetRetargetSourceCachedData(FName InRetargetSourceName, const TArray<FTransform>& InRetargetTransforms) const;

private:
    void Initialize();
    void RemapFromSkelMesh(const USkeletalMesh& SourceSkeletalMesh, USkeleton& TargetSkeleton);
    void RemapFromSkeleton(const USkeleton& SourceSkeleton);

    UObject* Asset = nullptr;
    USkeletalMesh* AssetSkeletalMesh = nullptr;
    USkeleton* AssetSkeleton = nullptr;
    const FReferenceSkeleton* RefSkeleton = nullptr;
    TArray<FBoneIndexType> BoneIndicesArray;
    TArray<bool> BoneSwitchArray;
    TArray<int32> SkeletonToPoseBoneIndexArray;
    TArray<int32> PoseToSkeletonBoneIndexArray;
    TArray<FCompactPoseBoneIndex> CompactPoseParentBones;
    TArray<int32> CompactPoseToSkeletonIndex;
    TArray<FCompactPoseBoneIndex> SkeletonToCompactPose;
    TArray<FVirtualBoneCompactPoseData> VirtualBoneCompactPoseData;
    mutable TMap<FName, FRetargetSourceCachedData> RetargetSourceCachedDataLUT;
    bool bDisableRetargeting = false;
    uint16 SerialNumber = 0;
};

}
