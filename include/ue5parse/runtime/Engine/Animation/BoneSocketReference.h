#pragma once

#include "ue5parse/runtime/Engine/Animation/BoneReference.h"
#include "ue5parse/runtime/Engine/BonePose.h"

#include "BoneSocketReference.generated.h"

namespace ue5parse::runtime {

struct FAnimInstanceProxy;

// FSocketReference: a socket of the mesh, resolved to its bone (UE's Animation/BoneSocketReference.h).
USTRUCT()
struct FSocketReference {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName SocketName;

    // Finds the socket on the proxy's mesh; reports a socket that the mesh does not have.
    void InitializeSocketInfo(const FAnimInstanceProxy* InAnimInstanceProxy);
    void InitialzeCompactBoneIndex(const FBoneContainer& RequiredBones);
    // Whether the mesh has the socket.
    bool HasValidSetup() const { return CachedSocketMeshBoneIndex.IsValid(); }
    // Whether the bone container has the bone of the socket.
    bool IsValidToEvaluate() const { return CachedSocketCompactBoneIndex.IsValid(); }
    FCompactPoseBoneIndex GetCachedSocketCompactBoneIndex() const { return CachedSocketCompactBoneIndex; }
    template <typename PoseType>
    FTransform GetAnimatedSocketTransform(FCSPose<PoseType>& InPose) const {
        if (!CachedSocketCompactBoneIndex.IsValid()) return FTransform::Identity;
        return CachedSocketLocalTransform * InPose.GetComponentSpaceTransform(CachedSocketCompactBoneIndex);
    }
    void InvalidateCachedBoneIndex() {
        CachedSocketMeshBoneIndex = FMeshPoseBoneIndex(INDEX_NONE);
        CachedSocketCompactBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
    }

private:
    FTransform CachedSocketLocalTransform;
    FMeshPoseBoneIndex CachedSocketMeshBoneIndex = FMeshPoseBoneIndex(INDEX_NONE);
    FCompactPoseBoneIndex CachedSocketCompactBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
};

// FBoneSocketTarget: a bone or a socket that a bone control aims at (UE's Animation/BoneSocketReference.h).
USTRUCT()
struct FBoneSocketTarget {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bUseSocket = false;
    UPROPERTY()
    FBoneReference BoneReference;
    UPROPERTY()
    FSocketReference SocketReference;

    void InitializeBoneReferences(const FBoneContainer& RequiredBones) {
        if (bUseSocket) {
            SocketReference.InitialzeCompactBoneIndex(RequiredBones);
            BoneReference.InvalidateCachedBoneIndex();
        } else {
            BoneReference.Initialize(RequiredBones);
            SocketReference.InvalidateCachedBoneIndex();
        }
    }
    void Initialize(const FAnimInstanceProxy* InAnimInstanceProxy) {
        if (bUseSocket) SocketReference.InitializeSocketInfo(InAnimInstanceProxy);
    }
    bool HasValidSetup() const { return bUseSocket ? SocketReference.HasValidSetup() : BoneReference.BoneIndex != INDEX_NONE; }
    bool HasTargetSetup() const { return !GetTargetSetup().isNone(); }
    FName GetTargetSetup() const { return bUseSocket ? SocketReference.SocketName : BoneReference.BoneName; }
    bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const {
        return bUseSocket ? SocketReference.IsValidToEvaluate() : BoneReference.IsValidToEvaluate(RequiredBones);
    }
    // The compact pose index of the bone, or of the socket's bone.
    FCompactPoseBoneIndex GetCompactPoseBoneIndex() const {
        return bUseSocket ? SocketReference.GetCachedSocketCompactBoneIndex() : BoneReference.CachedCompactPoseIndex;
    }

    // The component space transform of the target, moved by an offset in its space. Without a target that the pose
    // has, the offset is a world location.
    template <typename PoseType>
    FTransform GetTargetTransform(const FVector& TargetOffset, FCSPose<PoseType>& InPose, const FTransform& InComponentToWorld) const {
        FTransform OutTargetTransform;
        if (bUseSocket && SocketReference.IsValidToEvaluate()) {
            OutTargetTransform = SocketReference.GetAnimatedSocketTransform(InPose);
        } else if (!bUseSocket && BoneReference.HasValidSetup() && BoneReference.IsValidToEvaluate()) {
            OutTargetTransform = InPose.GetComponentSpaceTransform(BoneReference.CachedCompactPoseIndex);
        } else {
            OutTargetTransform.SetLocation(InComponentToWorld.InverseTransformPosition(TargetOffset));
            return OutTargetTransform;
        }
        OutTargetTransform.SetLocation(OutTargetTransform.TransformPosition(TargetOffset));
        return OutTargetTransform;
    }
    template <typename PoseType>
    FTransform GetTargetTransform(const FTransform& TargetOffset, FCSPose<PoseType>& InPose, const FTransform& InComponentToWorld) const {
        if (bUseSocket && SocketReference.IsValidToEvaluate()) return TargetOffset * SocketReference.GetAnimatedSocketTransform(InPose);
        if (!bUseSocket && BoneReference.HasValidSetup() && BoneReference.IsValidToEvaluate())
            return TargetOffset * InPose.GetComponentSpaceTransform(BoneReference.CachedCompactPoseIndex);
        return TargetOffset.GetRelativeTransform(InComponentToWorld);
    }
};

}
