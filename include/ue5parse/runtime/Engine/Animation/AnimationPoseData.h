#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

// FAnimationPoseData: the pose and curves that an evaluation writes (UE's Animation/AnimationPoseData.h). Attributes
// are not part of the runtime.
struct FAnimationPoseData {
    FAnimationPoseData(FCompactPose& InPose, FBlendedCurve& InCurve) : Pose(InPose), Curve(InCurve) {}
    FCompactPose& GetPose() { return Pose; }
    const FCompactPose& GetPose() const { return Pose; }
    FBlendedCurve& GetCurve() { return Curve; }
    const FBlendedCurve& GetCurve() const { return Curve; }

private:
    FCompactPose& Pose;
    FBlendedCurve& Curve;
};

}
