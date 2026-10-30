#pragma once

#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/Math/Transform.h"

namespace ue5parse::runtime {

// FCCDIKChainLink: a joint of a CCD IK chain, its component space and local transforms (UE's AnimationCore CCDIK.h).
struct FCCDIKChainLink {
    FTransform Transform;
    FTransform LocalTransform;
    int32 TransformIndex = INDEX_NONE;
    TArray<int32> ChildZeroLengthTransformIndices;
    double CurrentAngleDelta = 0.0;

    FCCDIKChainLink() = default;
    FCCDIKChainLink(const FTransform& InTransform, const FTransform& InLocalTransform, int32 InTransformIndex)
        : Transform(InTransform), LocalTransform(InLocalTransform), TransformIndex(InTransformIndex) {}
};

namespace AnimationCore {

// Cyclic coordinate descent: turns each joint between the root and the tip so that the tip moves toward the target,
// up to MaxIteration passes or until the tip is within Precision. The per joint limits are in degrees. True when a
// joint moved.
bool SolveCCDIK(TArray<FCCDIKChainLink>& InOutChain, const FVector& TargetPosition, float Precision, int32 MaxIteration, bool bStartFromTail,
                bool bEnableRotationLimit, const TArray<float>& RotationLimitPerJoints);

}

}
