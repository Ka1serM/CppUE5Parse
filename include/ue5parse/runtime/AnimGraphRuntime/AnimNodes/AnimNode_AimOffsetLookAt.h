#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"

#include "AnimNode_AimOffsetLookAt.generated.h"

namespace ue5parse::runtime {

// An aim offset whose input is the azimuth and elevation from a socket to a world location (UE's
// AnimNodes/AnimNode_AimOffsetLookAt.h).
USTRUCT()
struct FAnimNode_AimOffsetLookAt : public FAnimNode_BlendSpacePlayer {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink BasePose;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;
    UPROPERTY()
    FName SourceSocketName;
    UPROPERTY()
    FName PivotSocketName;
    UPROPERTY()
    FVector LookAtLocation = FVector::ZeroVector;
    UPROPERTY()
    FVector SocketAxis = FVector(1.0, 0.0, 0.0);
    UPROPERTY()
    float Alpha = 1.f;
    UPROPERTY()
    FVector CurrentBlendInput = FVector::ZeroVector;
    UPROPERTY()
    FBoneReference SocketBoneReference;
    UPROPERTY()
    FBoneReference PivotSocketBoneReference;
    UPROPERTY()
    bool bIsLODEnabled = false;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
    bool NeedsOnInitializeAnimInstance() const override { return true; }
    void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
    FVector GetPosition() const override { return CurrentBlendInput; }

private:
    void UpdateFromLookAtTarget(FPoseContext& LocalPoseContext);

    FTransform SocketLocalTransform;
    FTransform PivotSocketLocalTransform;
};

}
