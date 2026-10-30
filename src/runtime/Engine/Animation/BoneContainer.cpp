#include "ue5parse/runtime/Engine/BoneContainer.h"

#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"

// UE's BoneContainer.cpp (UE 5.3). Curve filtering and reference pose overrides are not part of the runtime.

namespace ue5parse::runtime {

namespace {
// UE's BONE_TRANS_RT_ORIENT_AND_SCALE_PRECISION.
constexpr double OrientAndScalePrecision = 0.001;
}

FBoneContainer::FBoneContainer(const TArray<FBoneIndexType>& InRequiredBoneIndexArray, UObject& InAsset) {
    InitializeTo(InRequiredBoneIndexArray, InAsset);
}

void FBoneContainer::InitializeTo(const TArray<FBoneIndexType>& InRequiredBoneIndexArray, UObject& InAsset) {
    BoneIndicesArray = InRequiredBoneIndexArray;
    Asset = &InAsset;
    Initialize();
    // UE skips 0, which a node's cache uses for "never built".
    ++SerialNumber;
    if (SerialNumber == 0) ++SerialNumber;
}

void FBoneContainer::Reset() {
    const uint16 PreviousSerialNumber = SerialNumber;
    *this = FBoneContainer();
    SerialNumber = PreviousSerialNumber;
}

void FBoneContainer::Initialize() {
    RefSkeleton = nullptr;
    AssetSkeletalMesh = Cast<USkeletalMesh>(Asset);
    AssetSkeleton = nullptr;
    if (AssetSkeletalMesh != nullptr) {
        RefSkeleton = &AssetSkeletalMesh->GetRefSkeleton();
        AssetSkeleton = AssetSkeletalMesh->GetSkeleton();
    } else if ((AssetSkeleton = Cast<USkeleton>(Asset)) != nullptr) {
        RefSkeleton = &AssetSkeleton->GetReferenceSkeleton();
    }
    if (RefSkeleton == nullptr || AssetSkeleton == nullptr) {
        // A mesh without a skeleton: nothing can play on it.
        Asset = nullptr;
        return;
    }

    const int32 MaxBones = FMath::Max<int32>(RefSkeleton->GetNum(), AssetSkeleton->GetReferenceSkeleton().GetNum());
    BoneSwitchArray.SetNum(MaxBones);
    for (bool& Bit : BoneSwitchArray) Bit = false;
    for (const FBoneIndexType BoneIndex : BoneIndicesArray)
        if (BoneIndex < MaxBones) BoneSwitchArray[BoneIndex] = true;

    if (AssetSkeletalMesh != nullptr) RemapFromSkelMesh(*AssetSkeletalMesh, *AssetSkeleton);
    else RemapFromSkeleton(*AssetSkeleton);

    const int32 NumReqBones = BoneIndicesArray.Num();
    CompactPoseParentBones.Empty(NumReqBones);
    CompactPoseToSkeletonIndex.SetNum(NumReqBones);
    SkeletonToCompactPose.Empty(SkeletonToPoseBoneIndexArray.Num());
    VirtualBoneCompactPoseData.Empty();

    TArray<int32> MeshIndexToCompactPoseIndex;
    MeshIndexToCompactPoseIndex.SetNum(PoseToSkeletonBoneIndexArray.Num());
    for (int32& Item : MeshIndexToCompactPoseIndex) Item = INDEX_NONE;
    for (int32 CompactBoneIndex = 0; CompactBoneIndex < NumReqBones; ++CompactBoneIndex) {
        const FBoneIndexType MeshPoseIndex = BoneIndicesArray[CompactBoneIndex];
        MeshIndexToCompactPoseIndex[MeshPoseIndex] = CompactBoneIndex;
        const int32 ParentIndex = GetParentBoneIndex(MeshPoseIndex);
        CompactPoseParentBones.Add(FCompactPoseBoneIndex(ParentIndex == INDEX_NONE ? INDEX_NONE : MeshIndexToCompactPoseIndex[ParentIndex]));
    }
    for (int32 CompactBoneIndex = 0; CompactBoneIndex < NumReqBones; ++CompactBoneIndex)
        CompactPoseToSkeletonIndex[CompactBoneIndex] = PoseToSkeletonBoneIndexArray[BoneIndicesArray[CompactBoneIndex]];
    for (const int32 PoseBoneIndex : SkeletonToPoseBoneIndexArray)
        SkeletonToCompactPose.Add(FCompactPoseBoneIndex(PoseBoneIndex != INDEX_NONE ? MeshIndexToCompactPoseIndex[PoseBoneIndex] : INDEX_NONE));
    for (const FVirtualBoneRefData& VBRefBone : RefSkeleton->GetVirtualBoneRefData()) {
        const int32 VBInd = MeshIndexToCompactPoseIndex[VBRefBone.VBRefSkelIndex];
        const int32 SourceInd = MeshIndexToCompactPoseIndex[VBRefBone.SourceRefSkelIndex];
        const int32 TargetInd = MeshIndexToCompactPoseIndex[VBRefBone.TargetRefSkelIndex];
        if (VBInd != INDEX_NONE && SourceInd != INDEX_NONE && TargetInd != INDEX_NONE)
            VirtualBoneCompactPoseData.Add(
                {FCompactPoseBoneIndex(VBInd), FCompactPoseBoneIndex(SourceInd), FCompactPoseBoneIndex(TargetInd)});
    }
    RetargetSourceCachedDataLUT.Empty();
}

void FBoneContainer::RemapFromSkelMesh(const USkeletalMesh& SourceSkeletalMesh, USkeleton& TargetSkeleton) {
    SkeletonToPoseBoneIndexArray = TargetSkeleton.FindOrAddSkeletonToMeshLinkup(&SourceSkeletalMesh);
    PoseToSkeletonBoneIndexArray = TargetSkeleton.FindOrAddMeshToSkeletonLinkup(&SourceSkeletalMesh);
}

void FBoneContainer::RemapFromSkeleton(const USkeleton& SourceSkeleton) {
    const int32 NumBones = SourceSkeleton.GetReferenceSkeleton().GetNum();
    SkeletonToPoseBoneIndexArray.SetNum(NumBones);
    PoseToSkeletonBoneIndexArray.SetNum(NumBones);
    for (int32 Index = 0; Index < NumBones; ++Index) SkeletonToPoseBoneIndexArray[Index] = PoseToSkeletonBoneIndexArray[Index] = Index;
}

void FBoneContainer::FillWithCompactRefPose(TArray<FTransform>& OutTransforms) const {
    const TArray<FTransform>& RefPoseTransforms = RefSkeleton->GetRefBonePose();
    OutTransforms.SetNum(BoneIndicesArray.Num());
    for (int32 CompactBoneIndex = 0; CompactBoneIndex < BoneIndicesArray.Num(); ++CompactBoneIndex)
        OutTransforms[CompactBoneIndex] = RefPoseTransforms[BoneIndicesArray[CompactBoneIndex]];
}

bool FBoneContainer::BoneIsChildOf(const FCompactPoseBoneIndex& BoneIndex, const FCompactPoseBoneIndex& ParentBoneIndex) const {
    for (FCompactPoseBoneIndex Parent = GetParentBoneIndex(BoneIndex); Parent.IsValid(); Parent = GetParentBoneIndex(Parent))
        if (Parent == ParentBoneIndex) return true;
    return false;
}

FCompactPoseBoneIndex FBoneContainer::MakeCompactPoseIndex(const FMeshPoseBoneIndex& BoneIndex) const {
    for (int32 CompactIndex = 0; CompactIndex < BoneIndicesArray.Num(); ++CompactIndex)
        if (BoneIndicesArray[CompactIndex] == BoneIndex.GetInt()) return FCompactPoseBoneIndex(CompactIndex);
    return FCompactPoseBoneIndex(INDEX_NONE);
}

const FRetargetSourceCachedData& FBoneContainer::GetRetargetSourceCachedData(FName InRetargetSourceName,
                                                                            const TArray<FTransform>& InRetargetTransforms) const {
    if (const FRetargetSourceCachedData* Found = RetargetSourceCachedDataLUT.Find(InRetargetSourceName)) return *Found;
    FRetargetSourceCachedData& Data = RetargetSourceCachedDataLUT.FindOrAdd(InRetargetSourceName);
    const TArray<FTransform>& PlayingOnRefSkeleton = GetRefPoseArray();
    const int32 CompactPoseNumBones = GetCompactPoseNumBones();
    for (int32 CompactBoneIndex = 0; CompactBoneIndex < CompactPoseNumBones; CompactBoneIndex++) {
        const int32 SkeletonBoneIndex = CompactPoseToSkeletonIndex[CompactBoneIndex];
        if (SkeletonBoneIndex == INDEX_NONE || !InRetargetTransforms.IsValidIndex(SkeletonBoneIndex) ||
            AssetSkeleton->GetBoneTranslationRetargetingMode(SkeletonBoneIndex, bDisableRetargeting) != EBoneTranslationRetargetingMode::OrientAndScale)
            continue;
        const FVector SourceSkelTrans = InRetargetTransforms[SkeletonBoneIndex].GetTranslation();
        const FVector TargetSkelTrans = PlayingOnRefSkeleton[BoneIndicesArray[CompactBoneIndex]].GetTranslation();
        if (SourceSkelTrans.Equals(TargetSkelTrans, OrientAndScalePrecision)) continue;
        const float SourceSkelTransLength = static_cast<float>(SourceSkelTrans.Size());
        const float TargetSkelTransLength = static_cast<float>(TargetSkelTrans.Size());
        if (FMath::IsNearlyZero(SourceSkelTransLength * TargetSkelTransLength)) continue;
        const FQuat DeltaRotation = FQuat::FindBetweenNormals(SourceSkelTrans / SourceSkelTransLength, TargetSkelTrans / TargetSkelTransLength);
        const int32 OrientAndScaleIndex =
            Data.OrientAndScaleData.Add({DeltaRotation, TargetSkelTransLength / SourceSkelTransLength, SourceSkelTrans, TargetSkelTrans});
        if (Data.CompactPoseIndexToOrientAndScaleIndex.Num() == 0) {
            Data.CompactPoseIndexToOrientAndScaleIndex.SetNum(CompactPoseNumBones);
            for (int32& Item : Data.CompactPoseIndexToOrientAndScaleIndex) Item = INDEX_NONE;
        }
        Data.CompactPoseIndexToOrientAndScaleIndex[CompactBoneIndex] = OrientAndScaleIndex;
    }
    return Data;
}

}
