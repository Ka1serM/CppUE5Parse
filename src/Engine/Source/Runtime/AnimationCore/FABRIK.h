#pragma once

#include "Engine/Source/Runtime/AnimationCore/BoneIndices.h"
#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

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

bool SolveFabrik(TArray<FFABRIKChainLink>& InOutChain, const FVector& TargetPosition, double MaximumReach, double Precision, int32 MaxIterations);

}

}
