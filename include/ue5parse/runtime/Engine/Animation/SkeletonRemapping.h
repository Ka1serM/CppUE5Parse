#pragma once

#include <utility>

#include "ue5parse/runtime/Engine/ReferenceSkeleton.h"

namespace ue5parse::runtime {

class USkeleton;

// FSkeletonRemapping: the bones of one skeleton on another by name, with the rotations that correct for their
// different reference poses (UE's Animation/SkeletonRemapping.h).
struct FSkeletonRemapping {
    FSkeletonRemapping() = default;
    FSkeletonRemapping(const USkeleton* InSourceSkeleton, const USkeleton* InTargetSkeleton);

    const USkeleton* GetSourceSkeleton() const { return SourceSkeleton; }
    const USkeleton* GetTargetSkeleton() const { return TargetSkeleton; }
    int32 GetTargetSkeletonBoneIndex(int32 SourceSkeletonBoneIndex) const {
        return SourceToTargetBoneIndexes.IsValidIndex(SourceSkeletonBoneIndex) ? SourceToTargetBoneIndexes[SourceSkeletonBoneIndex] : INDEX_NONE;
    }
    int32 GetSourceSkeletonBoneIndex(int32 TargetSkeletonBoneIndex) const {
        return TargetToSourceBoneIndexes.IsValidIndex(TargetSkeletonBoneIndex) ? TargetToSourceBoneIndexes[TargetSkeletonBoneIndex] : INDEX_NONE;
    }
    FTransform RetargetBoneTransformToTargetSkeleton(int32 TargetSkeletonBoneIndex, const FTransform& SourceTransform) const {
        return FTransform(RetargetBoneRotationToTargetSkeleton(TargetSkeletonBoneIndex, SourceTransform.GetRotation()),
                          RetargetBoneTranslationToTargetSkeleton(TargetSkeletonBoneIndex, SourceTransform.GetTranslation()),
                          SourceTransform.GetScale3D());
    }
    FVector RetargetBoneTranslationToTargetSkeleton(int32 TargetSkeletonBoneIndex, const FVector& SourceTranslation) const {
        return RetargetingTable[TargetSkeletonBoneIndex].first.RotateVector(SourceTranslation);
    }
    FQuat RetargetBoneRotationToTargetSkeleton(int32 TargetSkeletonBoneIndex, const FQuat& SourceRotation) const {
        const std::pair<FQuat, FQuat>& QQ = RetargetingTable[TargetSkeletonBoneIndex];
        return QQ.first * SourceRotation * QQ.second;
    }
    FVector RetargetAdditiveTranslationToTargetSkeleton(int32 TargetSkeletonBoneIndex, const FVector& SourceTranslation) const {
        return RetargetingTable[TargetSkeletonBoneIndex].first.RotateVector(SourceTranslation);
    }
    FQuat RetargetAdditiveRotationToTargetSkeleton(int32 TargetSkeletonBoneIndex, const FQuat& SourceRotation) const {
        const std::pair<FQuat, FQuat>& QQ = RetargetingTable[TargetSkeletonBoneIndex];
        return QQ.first * SourceRotation * QQ.first.Inverse();
    }
    bool RequiresReferencePoseRetarget() const { return RetargetingTable.Num() > 0; }
    bool IsValid() const { return SourceSkeleton != nullptr && TargetSkeleton != nullptr; }

private:
    void GenerateMapping();

    const USkeleton* SourceSkeleton = nullptr;
    const USkeleton* TargetSkeleton = nullptr;
    TArray<int32> SourceToTargetBoneIndexes;
    TArray<int32> TargetToSourceBoneIndexes;
    TArray<std::pair<FQuat, FQuat>> RetargetingTable;
};

namespace UE::Anim {
// FSkeletonRemappingRegistry: one remapping per skeleton pair, made on first use; the same skeleton maps to an empty
// (invalid) remapping.
class FSkeletonRemappingRegistry {
public:
    static FSkeletonRemappingRegistry& Get();
    const FSkeletonRemapping& GetRemapping(const USkeleton* InSourceSkeleton, const USkeleton* InTargetSkeleton);
};
}

}
