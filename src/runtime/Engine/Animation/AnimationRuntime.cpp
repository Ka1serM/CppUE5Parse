#include "ue5parse/runtime/Engine/AnimationRuntime.h"

#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Animation/SkeletonRemapping.h"

// UE's AnimationRuntime.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {
// UE's BONE_TRANS_RT_ORIENT_AND_SCALE_PRECISION.
constexpr double OrientAndScalePrecision = 0.001;
}

void FAnimationRuntime::RetargetBoneTransform(const USkeleton* SourceSkeleton, FName SourceName, const TArray<FTransform>& RetargetTransforms,
                                              FTransform& BoneTransform, int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex,
                                              const FBoneContainer& RequiredBones, bool bIsBakedAdditive) {
    if (SourceSkeleton == nullptr || RetargetTransforms.IsEmpty()) return;
    const USkeleton* TargetSkeleton = RequiredBones.GetSkeletonAsset();
    const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(SourceSkeleton, TargetSkeleton);
    const int32 TargetSkeletonBoneIndex = RequiredBones.GetSkeletonIndex(BoneIndex);
    const int32 SourceSkeletonBoneIndex =
        SkeletonRemapping.IsValid() ? SkeletonRemapping.GetSourceSkeletonBoneIndex(TargetSkeletonBoneIndex) : SkeletonBoneIndex;
    switch (TargetSkeleton->GetBoneTranslationRetargetingMode(TargetSkeletonBoneIndex, RequiredBones.GetDisableRetargeting())) {
    case EBoneTranslationRetargetingMode::AnimationScaled: {
        const float SourceTranslationLength = static_cast<float>(RetargetTransforms[SourceSkeletonBoneIndex].GetTranslation().Size());
        if (SourceTranslationLength > UE_KINDA_SMALL_NUMBER) {
            const float TargetTranslationLength = static_cast<float>(RequiredBones.GetRefPoseTransform(BoneIndex).GetTranslation().Size());
            BoneTransform.ScaleTranslation(TargetTranslationLength / SourceTranslationLength);
        }
        break;
    }
    case EBoneTranslationRetargetingMode::Skeleton:
        BoneTransform.SetTranslation(bIsBakedAdditive ? FVector::ZeroVector : RequiredBones.GetRefPoseTransform(BoneIndex).GetTranslation());
        break;
    case EBoneTranslationRetargetingMode::AnimationRelative:
        // A baked additive cancels the relative offset: (A1 + Rel) - (A2 + Rel) = A1 - A2.
        if (!bIsBakedAdditive) {
            const FTransform& RefPoseTransform = RequiredBones.GetRefPoseTransform(BoneIndex);
            FTransform BaseTransform = RetargetTransforms[SourceSkeletonBoneIndex];
            if (SkeletonRemapping.RequiresReferencePoseRetarget())
                BaseTransform = SkeletonRemapping.RetargetBoneTransformToTargetSkeleton(TargetSkeletonBoneIndex, BaseTransform);
            BoneTransform.SetRotation(BoneTransform.GetRotation() * BaseTransform.GetRotation().Inverse() * RefPoseTransform.GetRotation());
            BoneTransform.SetTranslation(BoneTransform.GetTranslation() + (RefPoseTransform.GetTranslation() - BaseTransform.GetTranslation()));
            BoneTransform.SetScale3D(BoneTransform.GetScale3D() * (RefPoseTransform.GetScale3D() * BaseTransform.GetSafeScaleReciprocal(BaseTransform.GetScale3D())));
            BoneTransform.NormalizeRotation();
        }
        break;
    case EBoneTranslationRetargetingMode::OrientAndScale:
        if (!bIsBakedAdditive) {
            const FRetargetSourceCachedData& CachedData = RequiredBones.GetRetargetSourceCachedData(SourceName, RetargetTransforms);
            if (CachedData.OrientAndScaleData.Num() > 0 && CachedData.CompactPoseIndexToOrientAndScaleIndex.Num() == RequiredBones.GetCompactPoseNumBones()) {
                const int32 OrientAndScaleIndex = CachedData.CompactPoseIndexToOrientAndScaleIndex[BoneIndex.GetInt()];
                if (OrientAndScaleIndex != INDEX_NONE) {
                    const FOrientAndScaleRetargetingCachedData& Data = CachedData.OrientAndScaleData[OrientAndScaleIndex];
                    const FVector AnimatedTranslation = BoneTransform.GetTranslation();
                    BoneTransform.SetTranslation((AnimatedTranslation - Data.SourceTranslation).IsNearlyZero(OrientAndScalePrecision)
                                                     ? Data.TargetTranslation
                                                     : Data.TranslationDeltaOrient.RotateVector(AnimatedTranslation) * Data.TranslationScale);
                }
            }
        }
        break;
    default: break;
    }
}

void FAnimationRuntime::FillUpComponentSpaceTransforms(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& BoneSpaceTransforms,
                                                       TArray<FTransform>& ComponentSpaceTransforms) {
    ComponentSpaceTransforms.SetNum(BoneSpaceTransforms.Num());
    if (ComponentSpaceTransforms.Num() == 0) return;
    ComponentSpaceTransforms[0] = BoneSpaceTransforms[0];
    for (int32 BoneIndex = 1; BoneIndex < ComponentSpaceTransforms.Num(); BoneIndex++)
        ComponentSpaceTransforms[BoneIndex] = BoneSpaceTransforms[BoneIndex] * ComponentSpaceTransforms[RefSkeleton.GetParentIndex(BoneIndex)];
}

}
