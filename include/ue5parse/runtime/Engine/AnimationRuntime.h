#pragma once

#include <span>

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"
#include "ue5parse/runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

class UBlendProfile;
class USkeleton;
enum EAdditiveAnimationType : int32;
struct FBlendSampleData;
struct FInputBlendPose;
struct FPerBoneBlendWeight;
namespace ECurveBlendOption {
enum Type : int32;
}

enum ETypeAdvanceAnim : int32 { ETAA_Default, ETAA_Finished, ETAA_Looped };

// The space a bone control works in.
enum EBoneControlSpace : uint8 { BCS_WorldSpace, BCS_ComponentSpace, BCS_ParentBoneSpace, BCS_BoneSpace };

// UE's FAnimationRuntime (AnimationRuntime.h): pose operations shared by the assets and the anim nodes.
class FAnimationRuntime {
public:
    // Applies the skeleton's translation retargeting mode of a bone to its sampled transform.
    static void RetargetBoneTransform(const USkeleton* SourceSkeleton, FName SourceName, const TArray<FTransform>& RetargetTransforms,
                                      FTransform& BoneTransform, int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex,
                                      const FBoneContainer& RequiredBones, bool bIsBakedAdditive);
    // Moves InOutTime by MoveDelta: a looping asset wraps around, another one stops at its ends.
    static ETypeAdvanceAnim AdvanceTime(bool bAllowLooping, float MoveDelta, float& InOutTime, float EndTime);

    // The weighted sum of the poses, with shortest-path rotations, normalized; the weights sum to 1.
    static void BlendPosesTogetherIndirect(std::span<const FCompactPose* const> SourcePoses, std::span<const FBlendedCurve* const> SourceCurves,
                                           std::span<const float> SourceWeights, FAnimationPoseData& OutAnimationPoseData);
    static void BlendTwoPosesTogether(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData, float WeightOfPoseOne,
                                      FAnimationPoseData& OutAnimationPoseData);
    // The poses blended by their samples' weights; a bone that a per bone smoothing covers (PerBoneIndices) takes the
    // sample's weight of that smoothing.
    static void BlendPosesTogetherPerBone(const TArray<FCompactPose>& SourcePoses, const TArray<FBlendedCurve>& SourceCurves,
                                          const TArray<int32>& PerBoneIndices, const TArray<FBlendSampleData>& BlendSampleDataCache,
                                          FAnimationPoseData& OutAnimationPoseData);
    // BlendPosesTogetherPerBone with the rotations blended in mesh space; the source poses change.
    static void BlendPosesTogetherPerBoneInMeshSpace(TArray<FCompactPose>& SourcePoses, const TArray<FBlendedCurve>& SourceCurves,
                                                     const TArray<int32>& PerBoneIndices, const TArray<FBlendSampleData>& BlendSampleDataCache,
                                                     FAnimationPoseData& OutAnimationPoseData);
    // PoseOne to PoseTwo by a weight per compact bone; the curves of PoseTwo override.
    static void BlendTwoPosesTogetherPerBone(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData,
                                             const TArray<float>& WeightsOfSource2, FAnimationPoseData& OutAnimationPoseData);
    // The source curves blended by weight: the first overrides, the others accumulate.
    static void BlendCurves(std::span<const FBlendedCurve* const> SourceCurves, std::span<const float> SourceWeights, FBlendedCurve& OutCurve);
    // The source curves combined by the blend option.
    static void BlendCurves(std::span<const FBlendedCurve* const> SourceCurves, std::span<const float> SourceWeights, FBlendedCurve& OutCurve,
                            ECurveBlendOption::Type BlendOption);

    enum class EBlendPosesPerBoneFilterFlags : uint32 { None = 0, MeshSpaceRotation = 1 << 0, MeshSpaceScale = 1 << 1 };
    // Each bone blends from the base pose to its source pose by its weight; rotations and scales optionally blend in
    // mesh space. Each pose's curves blend by the pose's highest bone weight.
    static void BlendPosesPerBoneFilter(FCompactPose& BasePose, const TArray<FCompactPose>& BlendPoses, FBlendedCurve& BaseCurve,
                                        const TArray<FBlendedCurve>& BlendedCurves, FAnimationPoseData& OutAnimationPoseData,
                                        const TArray<FPerBoneBlendWeight>& BoneBlendWeights, EBlendPosesPerBoneFilterFlags BlendFlags,
                                        ECurveBlendOption::Type CurveBlendOption);
    // The bone weights of the layers' weights: each source weight times its layer's weight, zero when not relevant.
    static void UpdateDesiredBoneWeight(const TArray<FPerBoneBlendWeight>& SrcBoneBlendWeights, TArray<FPerBoneBlendWeight>& TargetBoneBlendWeights,
                                        const TArray<float>& BlendWeights);
    // The weight of each skeleton bone from branch filters or blend masks; the last layer that reaches a bone owns it.
    static void CreateMaskWeights(TArray<FPerBoneBlendWeight>& BoneBlendWeights, const TArray<FInputBlendPose>& BlendFilters, const USkeleton* Skeleton);
    static void CreateMaskWeights(TArray<FPerBoneBlendWeight>& BoneBlendWeights, const TArray<UBlendProfile*>& BlendMasks, const USkeleton* Skeleton);

    // PoseA becomes (1 - Alpha) PoseA + Alpha PoseB.
    static void LerpPoses(FCompactPose& PoseA, const FCompactPose& PoseB, FBlendedCurve& CurveA, const FBlendedCurve& CurveB, float Alpha);
    // LerpPoses with Alpha times each compact bone's weight.
    static void LerpPosesPerBone(FCompactPose& PoseA, const FCompactPose& PoseB, FBlendedCurve& CurveA, const FBlendedCurve& CurveB, float Alpha,
                                 const TArray<float>& PerBoneWeights);

    // The target pose becomes its delta from the base pose.
    static void ConvertPoseToAdditive(FCompactPose& TargetPose, const FCompactPose& BasePose);
    static void ConvertTransformToAdditive(FTransform& TargetTransform, const FTransform& BaseTransform);
    // Rotations relative to the component instead of the parent, and back. Translations and scales stay local.
    static void ConvertPoseToMeshRotation(FCompactPose& LocalPose);
    static void ConvertMeshRotationPoseToLocalSpace(FCompactPose& Pose);
    // Adds the additive pose and curves to the base with the weight.
    static void AccumulateAdditivePose(FAnimationPoseData& BaseAnimationPoseData, const FAnimationPoseData& AdditiveAnimationPoseData, float Weight,
                                       EAdditiveAnimationType AdditiveType);
    static void AccumulateLocalSpaceAdditivePoseInternal(FCompactPose& BasePose, const FCompactPose& AdditivePose, float Weight);
    static void AccumulateMeshSpaceRotationAdditiveToLocalPoseInternal(FCompactPose& BasePose, const FCompactPose& MeshSpaceRotationAdditive,
                                                                       float Weight);

    // A component space bone transform to the control space, and back.
    static void ConvertCSTransformToBoneSpace(const FTransform& ComponentTransform, FCSPose<FCompactPose>& MeshBases, FTransform& InOutCSBoneTM,
                                              FCompactPoseBoneIndex BoneIndex, EBoneControlSpace Space);
    static void ConvertBoneSpaceTransformToCS(const FTransform& ComponentTransform, FCSPose<FCompactPose>& MeshBases, FTransform& InOutBoneSpaceTM,
                                              FCompactPoseBoneIndex BoneIndex, EBoneControlSpace Space);

    // The component space transform of a bone in the reference pose of the required bones.
    static FTransform GetComponentSpaceRefPose(const FCompactPoseBoneIndex& CompactPoseBoneIndex, const FBoneContainer& BoneContainer);
    static void FillUpComponentSpaceTransforms(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& BoneSpaceTransforms,
                                               TArray<FTransform>& ComponentSpaceTransforms);
};

inline FAnimationRuntime::EBlendPosesPerBoneFilterFlags operator|(FAnimationRuntime::EBlendPosesPerBoneFilterFlags A,
                                                                  FAnimationRuntime::EBlendPosesPerBoneFilterFlags B) {
    return FAnimationRuntime::EBlendPosesPerBoneFilterFlags(uint32(A) | uint32(B));
}
inline FAnimationRuntime::EBlendPosesPerBoneFilterFlags& operator|=(FAnimationRuntime::EBlendPosesPerBoneFilterFlags& A,
                                                                    FAnimationRuntime::EBlendPosesPerBoneFilterFlags B) {
    return A = A | B;
}
inline bool EnumHasAnyFlags(FAnimationRuntime::EBlendPosesPerBoneFilterFlags Flags, FAnimationRuntime::EBlendPosesPerBoneFilterFlags Contains) {
    return (uint32(Flags) & uint32(Contains)) != 0;
}

}
