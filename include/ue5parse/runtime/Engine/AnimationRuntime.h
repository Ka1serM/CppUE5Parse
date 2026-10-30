#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

class USkeleton;

// UE's FAnimationRuntime (AnimationRuntime.h): pose operations shared by the assets and the anim nodes.
class FAnimationRuntime {
public:
    // Applies the skeleton's translation retargeting mode of a bone to its sampled transform.
    static void RetargetBoneTransform(const USkeleton* SourceSkeleton, FName SourceName, const TArray<FTransform>& RetargetTransforms,
                                      FTransform& BoneTransform, int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex,
                                      const FBoneContainer& RequiredBones, bool bIsBakedAdditive);
    static void FillUpComponentSpaceTransforms(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& BoneSpaceTransforms,
                                               TArray<FTransform>& ComponentSpaceTransforms);
};

}
