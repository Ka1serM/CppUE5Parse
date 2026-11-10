#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"
#include "Engine/Source/Runtime/Engine/Animation/InputScaleBias.h"
#include "Engine/Source/Runtime/Engine/AnimationRuntime.h"

#include "AnimNode_BlendBoneByChannel.generated.h"

namespace ue5parse::runtime {

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
