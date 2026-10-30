#pragma once

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/BoneReference.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_BlendBoneByChannel.generated.h"

namespace ue5parse::runtime {

// A bone of pose B blended onto a bone of pose A, channel by channel.
USTRUCT()
struct FBlendBoneByChannelEntry {
    GENERATED_BODY()
    UPROPERTY()
    FBoneReference SourceBone;
    UPROPERTY()
    FBoneReference TargetBone;
    UPROPERTY()
    bool bBlendTranslation = true;
    UPROPERTY()
    bool bBlendRotation = true;
    UPROPERTY()
    bool bBlendScale = true;
};

// Blends chosen bones of B onto A by an alpha, in a chosen space (UE's AnimNodes/AnimNode_BlendBoneByChannel.h).
USTRUCT()
struct FAnimNode_BlendBoneByChannel : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink A;
    UPROPERTY()
    FPoseLink B;
    UPROPERTY()
    TArray<FBlendBoneByChannelEntry> BoneDefinitions;
    UPROPERTY()
    float Alpha = 0.0f;
    UPROPERTY()
    FInputScaleBias AlphaScaleBias;
    UPROPERTY()
    TEnumAsByte<EBoneControlSpace> TransformsSpace = BCS_BoneSpace;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

private:
    TArray<FBlendBoneByChannelEntry> ValidBoneEntries;
    float InternalBlendAlpha = 0.0f;
    bool bBIsRelevant = false;
};

}
