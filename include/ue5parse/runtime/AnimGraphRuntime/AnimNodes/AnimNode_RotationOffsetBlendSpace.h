#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_RotationOffsetBlendSpace.generated.h"

namespace ue5parse::runtime {

// An aim offset: a blend space of mesh space rotation additives applied over a base pose by an alpha (UE's
// AnimNodes/AnimNode_RotationOffsetBlendSpace.h).
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
