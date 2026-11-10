#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimSequenceDecompressionContext.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationAsset.h"
#include "Engine/Source/Runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

namespace ERootMotionRootLock {

enum Type : int32 { RefPose, AnimFirstFrame, Zero };

}

struct FRootMotionReset {
    FRootMotionReset(bool bInEnableRootMotion, ERootMotionRootLock::Type InRootMotionRootLock, bool bInForceRootLock, FTransform InAnimFirstFrame,
                     bool bInIsValidAdditive)
        : AnimFirstFrame(InAnimFirstFrame), RootMotionRootLock(InRootMotionRootLock), bEnableRootMotion(bInEnableRootMotion), bForceRootLock(bInForceRootLock),
          bIsValidAdditive(bInIsValidAdditive) {}

    FTransform AnimFirstFrame;
    ERootMotionRootLock::Type RootMotionRootLock;
    bool bEnableRootMotion;
    bool bForceRootLock;
    bool bIsValidAdditive;

    void ResetRootBoneForRootMotion(FTransform& BoneTransform, const FBoneContainer& RequiredBones) const;
};

namespace UE::Anim::Decompression {

void DecompressPose(FCompactPose& OutPose, const FCompressedAnimSequence& CompressedData, const FAnimExtractContext& ExtractionContext,
                    FAnimSequenceDecompressionContext& DecompressionContext, const TArray<FTransform>& RetargetTransforms,
                    const FRootMotionReset& RootMotionReset);

}

}
