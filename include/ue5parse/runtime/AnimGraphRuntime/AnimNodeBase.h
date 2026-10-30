#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"
#include "ue5parse/runtime/Engine/BoneContainer.h"

namespace ue5parse::runtime {

struct FAnimationUpdateContext {
    float DeltaTime = 0;
    float FinalBlendWeight = 1;
};

struct FPoseContext {
    FCompactPose Pose;
    FBlendedCurve Curve;
    bool bExpectsAdditivePose = false;

    explicit FPoseContext(const FBoneContainer& RequiredBones, bool bInExpectsAdditivePose = false)
        : bExpectsAdditivePose(bInExpectsAdditivePose) { Pose.SetBoneContainer(&RequiredBones); }

    void ResetToRefPose() {
        if (bExpectsAdditivePose) Pose.ResetToAdditiveIdentity();
        else Pose.ResetToRefPose();
        Curve.Empty();
    }
    FAnimationPoseData GetAnimationPoseData() { return FAnimationPoseData(Pose, Curve); }
};

struct FAnimNode_Base {
    virtual ~FAnimNode_Base() = default;
    virtual void Initialize_AnyThread() {}
    virtual void Update_AnyThread(const FAnimationUpdateContext& Context) = 0;
    virtual void Evaluate_AnyThread(FPoseContext& Output) = 0;
};

}
