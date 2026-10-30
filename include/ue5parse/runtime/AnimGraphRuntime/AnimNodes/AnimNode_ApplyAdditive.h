#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_ApplyAdditive.generated.h"

namespace ue5parse::runtime {

// Adds a local space additive pose to a base pose by an alpha (UE's AnimNodes/AnimNode_ApplyAdditive.h).
USTRUCT()
struct FAnimNode_ApplyAdditive : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink Base;
    UPROPERTY()
    FPoseLink Additive;
    UPROPERTY()
    float Alpha = 1.0f;
    UPROPERTY()
    FInputScaleBias AlphaScaleBias;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;
    UPROPERTY()
    FInputAlphaBoolBlend AlphaBoolBlend;
    UPROPERTY()
    FName AlphaCurveName;
    UPROPERTY()
    FInputScaleBiasClamp AlphaScaleBiasClamp;
    UPROPERTY()
    EAnimAlphaInputType AlphaInputType = EAnimAlphaInputType::Float;
    UPROPERTY()
    bool bAlphaBoolEnabled = true;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

private:
    float ActualAlpha = 0;
};

}
