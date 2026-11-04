#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "Engine/Source/Runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_RotationOffsetBlendSpace.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_RotationOffsetBlendSpace : public FAnimNode_BlendSpacePlayer {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink BasePose;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;
    UPROPERTY()
    float Alpha = 1.f;
    UPROPERTY()
    FInputScaleBias AlphaScaleBias;
    UPROPERTY()
    FInputAlphaBoolBlend AlphaBoolBlend;
    UPROPERTY()
    FName AlphaCurveName;
    UPROPERTY()
    FInputScaleBiasClamp AlphaScaleBiasClamp;
    UPROPERTY()
    float ActualAlpha = 0.0f;
    UPROPERTY()
    EAnimAlphaInputType AlphaInputType = EAnimAlphaInputType::Float;
    UPROPERTY()
    bool bAlphaBoolEnabled = false;
    UPROPERTY()
    bool bIsLODEnabled = false;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
};

}
