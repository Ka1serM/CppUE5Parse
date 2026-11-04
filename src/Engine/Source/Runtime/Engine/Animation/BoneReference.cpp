#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"

#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"
#include "Engine/Source/Runtime/Engine/BoneContainer.h"

namespace ue5parse::runtime {

bool FBoneReference::Initialize(const FBoneContainer& RequiredBones) {
    BoneIndex = RequiredBones.GetPoseBoneIndexForBoneName(BoneName);
    bUseSkeletonIndex = false;
    CachedCompactPoseIndex = BoneIndex == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : RequiredBones.MakeCompactPoseIndex(GetMeshPoseIndex(RequiredBones));
    return BoneIndex != INDEX_NONE;
}

bool FBoneReference::Initialize(const USkeleton* Skeleton) {
    BoneIndex = Skeleton != nullptr && !BoneName.isNone() ? Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName) : INDEX_NONE;
    bUseSkeletonIndex = BoneIndex != INDEX_NONE;
    CachedCompactPoseIndex = FCompactPoseBoneIndex(INDEX_NONE);
    return BoneIndex != INDEX_NONE;
}

bool FBoneReference::IsValidToEvaluate(const FBoneContainer& RequiredBones) const {
    if (!bUseSkeletonIndex) return CachedCompactPoseIndex != INDEX_NONE;
    return RequiredBones.GetCompactPoseIndexFromSkeletonIndex(BoneIndex) != INDEX_NONE;
}

FMeshPoseBoneIndex FBoneReference::GetMeshPoseIndex(const FBoneContainer& RequiredBones) const {
    if (bUseSkeletonIndex) return RequiredBones.GetMeshPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(BoneIndex));
    return FMeshPoseBoneIndex(BoneIndex);
}

FCompactPoseBoneIndex FBoneReference::GetCompactPoseIndex(const FBoneContainer& RequiredBones) const {
    if (bUseSkeletonIndex) return RequiredBones.GetCompactPoseIndexFromSkeletonIndex(BoneIndex);
    return CachedCompactPoseIndex;
}

}
