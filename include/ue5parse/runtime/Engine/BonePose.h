#pragma once

#include "ue5parse/runtime/Engine/BoneContainer.h"

// Poses of the compact bones (UE's BonePose.h): local space transforms (FCompactPose) and component space transforms
// computed on demand (FCSPose).

namespace ue5parse::runtime {

// FCompactPose: the local transform of each bone of a bone container, in compact order.
struct FCompactPose {
    void SetBoneContainer(const FBoneContainer* InBoneContainer) {
        BoneContainer = InBoneContainer;
        Bones.SetNum(InBoneContainer ? InBoneContainer->GetCompactPoseNumBones() : 0);
    }
    const FBoneContainer& GetBoneContainer() const { return *BoneContainer; }
    bool IsValid() const { return BoneContainer != nullptr && BoneContainer->IsValid(); }
    int32 GetNumBones() const { return Bones.Num(); }
    bool IsValidIndex(const FCompactPoseBoneIndex& BoneIndex) const { return Bones.IsValidIndex(BoneIndex.GetInt()); }
    FTransform& operator[](const FCompactPoseBoneIndex& BoneIndex) { return Bones[BoneIndex.GetInt()]; }
    const FTransform& operator[](const FCompactPoseBoneIndex& BoneIndex) const { return Bones[BoneIndex.GetInt()]; }
    TArray<FTransform>& GetMutableBones() { return Bones; }
    const TArray<FTransform>& GetBones() const { return Bones; }
    FCompactPoseBoneIndex GetParentBoneIndex(const FCompactPoseBoneIndex& BoneIndex) const { return BoneContainer->GetParentBoneIndex(BoneIndex); }
    const FTransform& GetRefPose(const FCompactPoseBoneIndex& BoneIndex) const { return BoneContainer->GetRefPoseTransform(BoneIndex); }
    void CopyBonesFrom(const FCompactPose& SrcPose) {
        Bones = SrcPose.Bones;
        BoneContainer = SrcPose.BoneContainer;
    }
    void MoveBonesFrom(FCompactPose& SrcPose) {
        Bones = std::move(SrcPose.Bones);
        BoneContainer = SrcPose.BoneContainer;
    }
    void Empty() {
        BoneContainer = nullptr;
        Bones.Empty();
    }

    void ResetToRefPose() { BoneContainer->FillWithCompactRefPose(Bones); }
    // Every bone the identity of an additive pose: no rotation, no translation, a scale delta of zero.
    void ResetToAdditiveIdentity();
    void NormalizeRotations();
    bool IsNormalized() const;

private:
    TArray<FTransform> Bones;
    const FBoneContainer* BoneContainer = nullptr;
};

// A component space transform for a bone (UE's FBoneTransform).
struct FBoneTransform {
    FCompactPoseBoneIndex BoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
    FTransform Transform;

    FBoneTransform() = default;
    FBoneTransform(FCompactPoseBoneIndex InBoneIndex, const FTransform& InTransform) : BoneIndex(InBoneIndex), Transform(InTransform) {}
};

struct FCompareBoneTransformIndex {
    bool operator()(const FBoneTransform& A, const FBoneTransform& B) const { return A.BoneIndex < B.BoneIndex; }
};

// FCSPose<FCompactPose>: a pose whose bones hold their component space transform once it is computed (flag 1), else
// their local transform (flag 0). The root starts computed.
template <class PoseType>
struct FCSPose {
    void InitPose(const FBoneContainer* InBoneContainer) {
        Pose.SetBoneContainer(InBoneContainer);
        Pose.ResetToRefPose();
        InitFlags();
    }
    void InitPose(const PoseType& SrcPose) {
        Pose.CopyBonesFrom(SrcPose);
        InitFlags();
    }
    void InitPose(PoseType&& SrcPose) {
        Pose.MoveBonesFrom(SrcPose);
        InitFlags();
    }
    const PoseType& GetPose() const { return Pose; }
    const FBoneContainer& GetBoneContainer() const { return Pose.GetBoneContainer(); }
    const TArray<uint8>& GetComponentSpaceFlags() const { return ComponentSpaceFlags; }

    FTransform GetLocalSpaceTransform(FCompactPoseBoneIndex BoneIndex) {
        if (ComponentSpaceFlags[BoneIndex.GetInt()]) {
            const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(BoneIndex);
            if (ParentIndex.IsValid()) return Pose[BoneIndex].GetRelativeTransform(GetComponentSpaceTransform(ParentIndex));
        }
        return Pose[BoneIndex];
    }
    const FTransform& GetComponentSpaceTransform(FCompactPoseBoneIndex BoneIndex) {
        if (ComponentSpaceFlags[BoneIndex.GetInt()] == 0) CalculateComponentSpaceTransform(BoneIndex);
        return Pose[BoneIndex];
    }
    void SetComponentSpaceTransform(FCompactPoseBoneIndex BoneIndex, const FTransform& NewTransform) {
        Pose[BoneIndex] = NewTransform;
        ComponentSpaceFlags[BoneIndex.GetInt()] = 1;
    }

    // Sets component transforms of bones, sorted parents first. The computed children of the bones go back to local
    // space first, so that they follow (UE's SafeSetCSBoneTransforms).
    void SafeSetCSBoneTransforms(const TArray<FBoneTransform>& BoneTransforms) {
        if (BoneTransforms.Num() == 0) return;
        TArray<FCompactPoseBoneIndex> BonesToConvert;
        const int32 MinIndex = BoneTransforms[0].BoneIndex.GetInt();
        for (const FBoneTransform& Transform : BoneTransforms)
            if (ComponentSpaceFlags[Transform.BoneIndex.GetInt()] == 1) BonesToConvert.Add(Transform.BoneIndex);
        const int32 FirstChildTransform = BonesToConvert.Num();
        for (int32 Index = MinIndex; Index < Pose.GetNumBones(); ++Index) {
            const FCompactPoseBoneIndex BoneIndex(Index);
            const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(BoneIndex);
            if (ComponentSpaceFlags[Index] == 1 && BonesToConvert.Contains(ParentIndex)) BonesToConvert.AddUnique(BoneIndex);
        }
        for (int32 Idx = BonesToConvert.Num() - 1; Idx >= FirstChildTransform; --Idx) ConvertBoneToLocalSpace(BonesToConvert[Idx]);
        for (const FBoneTransform& BoneTransform : BoneTransforms) {
            const FCompactPoseBoneIndex ParentBoneIndex = Pose.GetParentBoneIndex(BoneTransform.BoneIndex);
            if (ParentBoneIndex.IsValid() && ComponentSpaceFlags[ParentBoneIndex.GetInt()] == 0) CalculateComponentSpaceTransform(ParentBoneIndex);
            SetComponentSpaceTransform(BoneTransform.BoneIndex, BoneTransform.Transform);
        }
    }

    // SafeSetCSBoneTransforms with a weight: the bones blend in local space between their old and new transforms (UE's
    // LocalBlendCSBoneTransforms).
    void LocalBlendCSBoneTransforms(const TArray<FBoneTransform>& BoneTransforms, float Alpha) {
        if (Alpha < ZERO_ANIMWEIGHT_THRESH) return;
        if (Alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH) {
            SafeSetCSBoneTransforms(BoneTransforms);
            return;
        }
        TArray<uint8> BoneMask;
        BoneMask.SetNum(Pose.GetNumBones());
        for (uint8& Bit : BoneMask) Bit = 0;
        TArray<FBoneTransform> LocalBoneTransforms;
        for (const FBoneTransform& BoneTransform : BoneTransforms) {
            LocalBoneTransforms.Add(FBoneTransform(BoneTransform.BoneIndex, GetLocalSpaceTransform(BoneTransform.BoneIndex)));
            BoneMask[BoneTransform.BoneIndex.GetInt()] = 1;
        }
        SafeSetCSBoneTransforms(BoneTransforms);
        for (int32 Index = 1; Index < Pose.GetNumBones(); ++Index) {
            const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(FCompactPoseBoneIndex(Index));
            if (ParentIndex.IsValid()) BoneMask[Index] |= BoneMask[ParentIndex.GetInt()];
        }
        for (int32 Index = Pose.GetNumBones() - 1; Index > 0; --Index)
            if (BoneMask[Index] != 0) ConvertBoneToLocalSpace(FCompactPoseBoneIndex(Index));
        for (const FBoneTransform& Local : LocalBoneTransforms) Pose[Local.BoneIndex].BlendWith(Local.Transform, 1.0f - Alpha);
    }

    static void ConvertComponentPosesToLocalPoses(const FCSPose& InPose, PoseType& OutPose) {
        OutPose.CopyBonesFrom(InPose.Pose);
        ToLocal(InPose.ComponentSpaceFlags, OutPose);
    }
    static void ConvertComponentPosesToLocalPoses(FCSPose&& InPose, PoseType& OutPose) {
        OutPose.MoveBonesFrom(InPose.Pose);
        ToLocal(InPose.ComponentSpaceFlags, OutPose);
    }

    // As ConvertComponentPosesToLocalPoses, for a pose whose computed bones can have parents that are not computed: those
    // parents are computed first. OutPose keeps its transforms of the bones that are not computed.
    static void ConvertComponentPosesToLocalPosesSafe(FCSPose& InPose, PoseType& OutPose) {
        if (InPose.ComponentSpaceFlags[0]) OutPose[FCompactPoseBoneIndex(0)] = InPose.Pose[FCompactPoseBoneIndex(0)];
        for (int32 Index = InPose.Pose.GetNumBones() - 1; Index > 0; Index--) {
            const FCompactPoseBoneIndex BoneIndex(Index);
            if (!InPose.ComponentSpaceFlags[Index]) continue;
            const FCompactPoseBoneIndex ParentIndex = OutPose.GetParentBoneIndex(BoneIndex);
            if (!InPose.ComponentSpaceFlags[ParentIndex.GetInt()]) InPose.CalculateComponentSpaceTransform(ParentIndex);
            OutPose[BoneIndex] = InPose.Pose[BoneIndex];
            OutPose[BoneIndex].SetToRelativeTransform(InPose.Pose[ParentIndex]);
            OutPose[BoneIndex].NormalizeRotation();
        }
    }

private:
    void InitFlags() {
        ComponentSpaceFlags.SetNum(Pose.GetNumBones());
        for (uint8& Flag : ComponentSpaceFlags) Flag = 0;
        if (ComponentSpaceFlags.Num() > 0) ComponentSpaceFlags[0] = 1;
    }
    static void ToLocal(const TArray<uint8>& Flags, PoseType& OutPose) {
        for (int32 Index = OutPose.GetNumBones() - 1; Index > 0; Index--) {
            const FCompactPoseBoneIndex BoneIndex(Index);
            if (!Flags[Index]) continue;
            OutPose[BoneIndex].SetToRelativeTransform(OutPose[OutPose.GetParentBoneIndex(BoneIndex)]);
            OutPose[BoneIndex].NormalizeRotation();
        }
    }
    void CalculateComponentSpaceTransform(FCompactPoseBoneIndex BoneIndex) {
        const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(BoneIndex);
        if (ComponentSpaceFlags[ParentIndex.GetInt()] == 0) CalculateComponentSpaceTransform(ParentIndex);
        Pose[BoneIndex] = Pose[BoneIndex] * Pose[ParentIndex];
        Pose[BoneIndex].NormalizeRotation();
        ComponentSpaceFlags[BoneIndex.GetInt()] = 1;
    }
    void ConvertBoneToLocalSpace(FCompactPoseBoneIndex BoneIndex) {
        if (BoneIndex.IsRootBone() || ComponentSpaceFlags[BoneIndex.GetInt()] != 1) return;
        Pose[BoneIndex].SetToRelativeTransform(Pose[Pose.GetParentBoneIndex(BoneIndex)]);
        ComponentSpaceFlags[BoneIndex.GetInt()] = 0;
    }

    PoseType Pose;
    TArray<uint8> ComponentSpaceFlags;
};

}
