#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_TwoWayBlend.generated.h"

namespace ue5parse::runtime {

// Blends A to B by an alpha (UE's AnimNodes/AnimNode_TwoWayBlend.h). An input with no weight is not updated.
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
