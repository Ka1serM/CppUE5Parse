#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_TwoWayBlend.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_TwoWayBlend : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink A;
    UPROPERTY()
    FPoseLink B;
    UPROPERTY()
    EAnimAlphaInputType AlphaInputType = EAnimAlphaInputType::Float;
    UPROPERTY()
    bool bAlphaBoolEnabled = false;
    UPROPERTY()
    bool bResetChildOnActivation = false;
    UPROPERTY()
    float Alpha = 0;
    UPROPERTY()
    FInputScaleBias AlphaScaleBias;
    UPROPERTY()
    FInputAlphaBoolBlend AlphaBoolBlend;
    UPROPERTY()
    FName AlphaCurveName;
    UPROPERTY()
    FInputScaleBiasClamp AlphaScaleBiasClamp;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

private:
    float InternalBlendAlpha = 0;
    bool bAIsRelevant = false;
    bool bBIsRelevant = false;
};

}
