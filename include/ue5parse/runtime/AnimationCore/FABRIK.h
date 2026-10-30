#pragma once

#include "ue5parse/runtime/AnimationCore/BoneIndices.h"
#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

// FFABRIKChainLink: a joint of a FABRIK chain, its position and its distance to its parent (UE's AnimationCore
// FABRIK.h).
struct FFABRIKChainLink {
    FVector Position = FVector::ZeroVector;
    double Length = 0.0;
    int32 BoneIndex = INDEX_NONE;
    int32 TransformIndex = INDEX_NONE;
    FVector DefaultDirToParent = FVector(-1.0, 0.0, 0.0);
    TArray<int32> ChildZeroLengthTransformIndices;

    FFABRIKChainLink() = default;
    FFABRIKChainLink(const FVector& InPosition, double InLength, FCompactPoseBoneIndex InBoneIndex, int32 InTransformIndex)
        : Position(InPosition), Length(InLength), BoneIndex(InBoneIndex.GetInt()), TransformIndex(InTransformIndex) {}
    FFABRIKChainLink(const FVector& InPosition, double InLength, FCompactPoseBoneIndex InBoneIndex, int32 InTransformIndex,
                     const FVector& InDefaultDirToParent)
        : Position(InPosition), Length(InLength), BoneIndex(InBoneIndex.GetInt()), TransformIndex(InTransformIndex),
          DefaultDirToParent(InDefaultDirToParent) {}
    FFABRIKChainLink(const FVector& InPosition, double InLength, int32 InBoneIndex, int32 InTransformIndex)
        : Position(InPosition), Length(InLength), BoneIndex(InBoneIndex), TransformIndex(InTransformIndex) {}
};

namespace AnimationCore {

// Forward and backward reaching IK: moves the joint positions so that the tip reaches the target (a target out of
// reach straightens the chain toward it). True when a joint moved.
bool SolveFabrik(TArray<FFABRIKChainLink>& InOutChain, const FVector& TargetPosition, double MaximumReach, double Precision, int32 MaxIterations);

}

}
