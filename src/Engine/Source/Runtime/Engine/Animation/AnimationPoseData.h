#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

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
