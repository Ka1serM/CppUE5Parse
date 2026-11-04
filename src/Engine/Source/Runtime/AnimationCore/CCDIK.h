#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Math/Transform.h"

namespace ue5parse::runtime {

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

bool SolveCCDIK(TArray<FCCDIKChainLink>& InOutChain, const FVector& TargetPosition, float Precision, int32 MaxIteration, bool bStartFromTail,
                bool bEnableRotationLimit, const TArray<float>& RotationLimitPerJoints);

}

}
