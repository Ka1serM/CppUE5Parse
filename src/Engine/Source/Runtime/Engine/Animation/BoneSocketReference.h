#pragma once

#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"
#include "Engine/Source/Runtime/Engine/BonePose.h"

#include "BoneSocketReference.generated.h"

namespace ue5parse::runtime {

class FAnimInstanceProxy;

USTRUCT()
struct FSocketReference {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName SocketName;

    void InitializeSocketInfo(const FAnimInstanceProxy* InAnimInstanceProxy);
    void InitialzeCompactBoneIndex(const FBoneContainer& RequiredBones);
    bool HasValidSetup() const { return CachedSocketMeshBoneIndex.IsValid(); }
    bool IsValidToEvaluate() const { return CachedSocketCompactBoneIndex.IsValid(); }
    FCompactPoseBoneIndex GetCachedSocketCompactBoneIndex() const { return CachedSocketCompactBoneIndex; }
    template <typename PoseType>
    FTransform GetAnimatedSocketTransform(FCSPose<PoseType>& InPose) const {
        if (!CachedSocketCompactBoneIndex.IsValid())
            return FTransform::Identity;
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
        if (bUseSocket)
            SocketReference.InitializeSocketInfo(InAnimInstanceProxy);
    }
    bool HasValidSetup() const { return bUseSocket ? SocketReference.HasValidSetup() : BoneReference.BoneIndex != INDEX_NONE; }
    bool HasTargetSetup() const { return !GetTargetSetup().isNone(); }
    FName GetTargetSetup() const { return bUseSocket ? SocketReference.SocketName : BoneReference.BoneName; }
    bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const {
        return bUseSocket ? SocketReference.IsValidToEvaluate() : BoneReference.IsValidToEvaluate(RequiredBones);
    }
    FCompactPoseBoneIndex GetCompactPoseBoneIndex() const {
        return bUseSocket ? SocketReference.GetCachedSocketCompactBoneIndex() : BoneReference.CachedCompactPoseIndex;
    }

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
        if (bUseSocket && SocketReference.IsValidToEvaluate())
            return TargetOffset * SocketReference.GetAnimatedSocketTransform(InPose);
        if (!bUseSocket && BoneReference.HasValidSetup() && BoneReference.IsValidToEvaluate())
            return TargetOffset * InPose.GetComponentSpaceTransform(BoneReference.CachedCompactPoseIndex);
        return TargetOffset.GetRelativeTransform(InComponentToWorld);
    }
};

}
