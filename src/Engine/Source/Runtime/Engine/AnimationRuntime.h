#pragma once

#include <span>

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationPoseData.h"
#include "Engine/Source/Runtime/Engine/BonePose.h"

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

enum EBoneControlSpace : uint8 { BCS_WorldSpace, BCS_ComponentSpace, BCS_ParentBoneSpace, BCS_BoneSpace };

class FAnimationRuntime {
public:
    static void RetargetBoneTransform(const USkeleton* SourceSkeleton, FName SourceName, const TArray<FTransform>& RetargetTransforms,
                                      FTransform& BoneTransform, int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex,
                                      const FBoneContainer& RequiredBones, bool bIsBakedAdditive);
    static ETypeAdvanceAnim AdvanceTime(bool bAllowLooping, float MoveDelta, float& InOutTime, float EndTime);

    static void BlendPosesTogetherIndirect(std::span<const FCompactPose* const> SourcePoses, std::span<const FBlendedCurve* const> SourceCurves,
                                           std::span<const float> SourceWeights, FAnimationPoseData& OutAnimationPoseData);
    static void BlendTwoPosesTogether(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData, float WeightOfPoseOne,
                                      FAnimationPoseData& OutAnimationPoseData);
    static void BlendPosesTogetherPerBone(const TArray<FCompactPose>& SourcePoses, const TArray<FBlendedCurve>& SourceCurves,
                                          const TArray<int32>& PerBoneIndices, const TArray<FBlendSampleData>& BlendSampleDataCache,
                                          FAnimationPoseData& OutAnimationPoseData);
    static void BlendPosesTogetherPerBoneInMeshSpace(TArray<FCompactPose>& SourcePoses, const TArray<FBlendedCurve>& SourceCurves,
                                                     const TArray<int32>& PerBoneIndices, const TArray<FBlendSampleData>& BlendSampleDataCache,
                                                     FAnimationPoseData& OutAnimationPoseData);
    static void BlendTwoPosesTogetherPerBone(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData,
                                             const TArray<float>& WeightsOfSource2, FAnimationPoseData& OutAnimationPoseData);
    static void BlendCurves(std::span<const FBlendedCurve* const> SourceCurves, std::span<const float> SourceWeights, FBlendedCurve& OutCurve);
    static void BlendCurves(std::span<const FBlendedCurve* const> SourceCurves, std::span<const float> SourceWeights, FBlendedCurve& OutCurve,
                            ECurveBlendOption::Type BlendOption);

    enum class EBlendPosesPerBoneFilterFlags : uint32 { None = 0, MeshSpaceRotation = 1 << 0, MeshSpaceScale = 1 << 1 };
    static void BlendPosesPerBoneFilter(FCompactPose& BasePose, const TArray<FCompactPose>& BlendPoses, FBlendedCurve& BaseCurve,
                                        const TArray<FBlendedCurve>& BlendedCurves, FAnimationPoseData& OutAnimationPoseData,
                                        const TArray<FPerBoneBlendWeight>& BoneBlendWeights, EBlendPosesPerBoneFilterFlags BlendFlags,
                                        ECurveBlendOption::Type CurveBlendOption);
    static void UpdateDesiredBoneWeight(const TArray<FPerBoneBlendWeight>& SrcBoneBlendWeights, TArray<FPerBoneBlendWeight>& TargetBoneBlendWeights,
                                        const TArray<float>& BlendWeights);
    static void CreateMaskWeights(TArray<FPerBoneBlendWeight>& BoneBlendWeights, const TArray<FInputBlendPose>& BlendFilters, const USkeleton* Skeleton);
    static void CreateMaskWeights(TArray<FPerBoneBlendWeight>& BoneBlendWeights, const TArray<TObjectPtr<UBlendProfile>>& BlendMasks,
                                  const USkeleton* Skeleton);

    static void ConvertPoseToAdditive(FCompactPose& TargetPose, const FCompactPose& BasePose);
    static void ConvertTransformToAdditive(FTransform& TargetTransform, const FTransform& BaseTransform);
    static void ConvertPoseToMeshRotation(FCompactPose& LocalPose);
    static void ConvertMeshRotationPoseToLocalSpace(FCompactPose& Pose);
    static void AccumulateAdditivePose(FAnimationPoseData& BaseAnimationPoseData, const FAnimationPoseData& AdditiveAnimationPoseData, float Weight,
                                       EAdditiveAnimationType AdditiveType);
    static void AccumulateLocalSpaceAdditivePoseInternal(FCompactPose& BasePose, const FCompactPose& AdditivePose, float Weight);
    static void AccumulateMeshSpaceRotationAdditiveToLocalPoseInternal(FCompactPose& BasePose, const FCompactPose& MeshSpaceRotationAdditive, float Weight);

    static void ConvertCSTransformToBoneSpace(const FTransform& ComponentTransform, FCSPose<FCompactPose>& MeshBases, FTransform& InOutCSBoneTM,
                                              FCompactPoseBoneIndex BoneIndex, EBoneControlSpace Space);
    static void ConvertBoneSpaceTransformToCS(const FTransform& ComponentTransform, FCSPose<FCompactPose>& MeshBases, FTransform& InOutBoneSpaceTM,
                                              FCompactPoseBoneIndex BoneIndex, EBoneControlSpace Space);

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
