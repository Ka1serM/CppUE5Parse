#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimSequenceDecompressionContext.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"
#include "ue5parse/runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

namespace ERootMotionRootLock {
enum Type : int32 { RefPose, AnimFirstFrame, Zero };
}

// FRootMotionReset: how the root bone is locked when root motion is extracted (UE's AnimCompressionTypes.h).
struct FRootMotionReset {
    FRootMotionReset(bool bInEnableRootMotion, ERootMotionRootLock::Type InRootMotionRootLock, bool bInForceRootLock, FTransform InAnimFirstFrame,
                     bool bInIsValidAdditive)
        : AnimFirstFrame(InAnimFirstFrame), RootMotionRootLock(InRootMotionRootLock), bEnableRootMotion(bInEnableRootMotion),
          bForceRootLock(bInForceRootLock), bIsValidAdditive(bInIsValidAdditive) {}

    FTransform AnimFirstFrame;
    ERootMotionRootLock::Type RootMotionRootLock;
    bool bEnableRootMotion;
    bool bForceRootLock;
    bool bIsValidAdditive;

    void ResetRootBoneForRootMotion(FTransform& BoneTransform, const FBoneContainer& RequiredBones) const;
};

namespace UE::Anim::Decompression {
// Samples the compressed tracks into the pose and applies translation retargeting (UE's DecompressPose).
void DecompressPose(FCompactPose& OutPose, const FCompressedAnimSequence& CompressedData, const FAnimExtractContext& ExtractionContext,
                    FAnimSequenceDecompressionContext& DecompressionContext, const TArray<FTransform>& RetargetTransforms,
                    const FRootMotionReset& RootMotionReset);
}

}
