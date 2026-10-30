#include "ue5parse/runtime/Engine/Animation/SkeletonRemapping.h"

#include <map>
#include <memory>
#include <mutex>

#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"

// UE's SkeletonRemapping.cpp and SkeletonRemappingRegistry.cpp (UE 5.3).

namespace ue5parse::runtime {

FSkeletonRemapping::FSkeletonRemapping(const USkeleton* InSourceSkeleton, const USkeleton* InTargetSkeleton)
    : SourceSkeleton(InSourceSkeleton), TargetSkeleton(InTargetSkeleton) {
    GenerateMapping();
}

void FSkeletonRemapping::GenerateMapping() {
    if (!IsValid()) return;
    const FReferenceSkeleton& SourceReferenceSkeleton = SourceSkeleton->GetReferenceSkeleton();
    const FReferenceSkeleton& TargetReferenceSkeleton = TargetSkeleton->GetReferenceSkeleton();
    const int32 SourceNumBones = SourceReferenceSkeleton.GetNum();
    const int32 TargetNumBones = TargetReferenceSkeleton.GetNum();
    if (SourceNumBones == 0 || TargetNumBones == 0) return;
    SourceToTargetBoneIndexes.SetNum(SourceNumBones);
    TargetToSourceBoneIndexes.SetNum(TargetNumBones);
    RetargetingTable.SetNum(TargetNumBones);
    for (int32 SourceBoneIndex = 0; SourceBoneIndex < SourceNumBones; ++SourceBoneIndex)
        SourceToTargetBoneIndexes[SourceBoneIndex] = TargetReferenceSkeleton.FindBoneIndex(SourceReferenceSkeleton.GetBoneName(SourceBoneIndex));
    SourceToTargetBoneIndexes[0] = 0;
    for (int32& Index : TargetToSourceBoneIndexes) Index = INDEX_NONE;
    for (int32 SourceBoneIndex = 0; SourceBoneIndex < SourceNumBones; ++SourceBoneIndex)
        if (SourceToTargetBoneIndexes[SourceBoneIndex] != INDEX_NONE) TargetToSourceBoneIndexes[SourceToTargetBoneIndexes[SourceBoneIndex]] = SourceBoneIndex;

    const TArray<FTransform>& SourceLocalTransforms = SourceReferenceSkeleton.GetRefBonePose();
    const TArray<FTransform>& TargetLocalTransforms = TargetReferenceSkeleton.GetRefBonePose();
    TArray<FTransform> SourceComponentTransforms;
    TArray<FTransform> TargetComponentTransforms;
    FAnimationRuntime::FillUpComponentSpaceTransforms(SourceReferenceSkeleton, SourceLocalTransforms, SourceComponentTransforms);
    FAnimationRuntime::FillUpComponentSpaceTransforms(TargetReferenceSkeleton, TargetLocalTransforms, TargetComponentTransforms);
    RetargetingTable[0] = {FQuat::Identity, SourceLocalTransforms[0].GetRotation().Inverse() * TargetLocalTransforms[0].GetRotation()};
    for (int32 TargetBoneIndex = 1; TargetBoneIndex < TargetNumBones; ++TargetBoneIndex) {
        const int32 SourceBoneIndex = TargetToSourceBoneIndexes[TargetBoneIndex];
        const int32 SourceParentIndex = SourceBoneIndex != INDEX_NONE ? SourceReferenceSkeleton.GetParentIndex(SourceBoneIndex) : INDEX_NONE;
        const int32 TargetParentIndex = TargetReferenceSkeleton.GetParentIndex(TargetBoneIndex);
        if (SourceParentIndex == INDEX_NONE || TargetParentIndex == INDEX_NONE) {
            RetargetingTable[TargetBoneIndex] = {FQuat::Identity, FQuat::Identity};
            continue;
        }
        const FQuat PS = SourceComponentTransforms[SourceParentIndex].GetRotation();
        const FQuat PT = TargetComponentTransforms[TargetParentIndex].GetRotation();
        const FQuat RS = SourceLocalTransforms[SourceBoneIndex].GetRotation();
        const FQuat RT = TargetLocalTransforms[TargetBoneIndex].GetRotation();
        RetargetingTable[TargetBoneIndex] = {PT.Inverse() * PS, RS.Inverse() * PS.Inverse() * PT * RT};
    }
}

namespace UE::Anim {

FSkeletonRemappingRegistry& FSkeletonRemappingRegistry::Get() {
    static FSkeletonRemappingRegistry Registry;
    return Registry;
}

const FSkeletonRemapping& FSkeletonRemappingRegistry::GetRemapping(const USkeleton* InSourceSkeleton, const USkeleton* InTargetSkeleton) {
    static const FSkeletonRemapping DefaultMapping;
    if (InSourceSkeleton == InTargetSkeleton) return DefaultMapping;
    static std::mutex Mutex;
    static std::map<std::pair<const USkeleton*, const USkeleton*>, std::unique_ptr<FSkeletonRemapping>> Mappings;
    std::lock_guard Lock(Mutex);
    std::unique_ptr<FSkeletonRemapping>& Mapping = Mappings[{InSourceSkeleton, InTargetSkeleton}];
    if (!Mapping) Mapping = std::make_unique<FSkeletonRemapping>(InSourceSkeleton, InTargetSkeleton);
    return *Mapping;
}

}

}
