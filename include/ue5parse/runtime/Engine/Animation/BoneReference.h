#pragma once

#include "ue5parse/runtime/AnimationCore/BoneIndices.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "BoneReference.generated.h"

namespace ue5parse::runtime {

struct FBoneContainer;
class USkeleton;

// FBoneReference: a bone by name, resolved against a bone container (UE's Animation/BoneReference.h).
USTRUCT()
struct FBoneReference {
    GENERATED_BODY()
    UPROPERTY()
    FName BoneName;

    int32 BoneIndex = INDEX_NONE;
    FCompactPoseBoneIndex CachedCompactPoseIndex = FCompactPoseBoneIndex(INDEX_NONE);
    bool bUseSkeletonIndex = false;

    FBoneReference() = default;
    FBoneReference(FName InBoneName) : BoneName(InBoneName) {}
    bool operator==(const FBoneReference& Other) const { return BoneName == Other.BoneName; }

    // Finds the bone in the container's mesh (or skeleton); false when it is not there.
    bool Initialize(const FBoneContainer& RequiredBones);
    bool Initialize(const USkeleton* Skeleton);
    void InvalidateCachedBoneIndex() {
        BoneIndex = INDEX_NONE;
        CachedCompactPoseIndex = FCompactPoseBoneIndex(INDEX_NONE);
    }
    // Whether the bone was found and the container needs it.
    bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const;
    bool IsValidToEvaluate() const { return CachedCompactPoseIndex != INDEX_NONE; }
    bool HasValidSetup() const { return BoneIndex != INDEX_NONE; }
    FMeshPoseBoneIndex GetMeshPoseIndex(const FBoneContainer& RequiredBones) const;
    FCompactPoseBoneIndex GetCompactPoseIndex(const FBoneContainer& RequiredBones) const;
};

}
