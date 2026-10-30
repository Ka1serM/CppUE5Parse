#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_ApplyMeshSpaceAdditive.generated.h"

namespace ue5parse::runtime {

// Adds a mesh space rotation additive to a base pose by an alpha (UE's Animation/AnimNode_ApplyMeshSpaceAdditive.h,
// Engine module).
USTRUCT()
struct FAnimNode_ApplyMeshSpaceAdditive : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink Base;
    UPROPERTY()
    FPoseLink Additive;
    UPROPERTY()
    EAnimAlphaInputType AlphaInputType = EAnimAlphaInputType::Float;
    UPROPERTY()
    float Alpha = 1.0f;
    UPROPERTY()
    bool bAlphaBoolEnabled = true;
    UPROPERTY()
    FInputAlphaBoolBlend AlphaBoolBlend;
    UPROPERTY()
    FName AlphaCurveName;
    UPROPERTY()
    FInputScaleBias AlphaScaleBias;
    UPROPERTY()
    FInputScaleBiasClamp AlphaScaleBiasClamp;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;
    UPROPERTY()
    float ActualAlpha = 0.f;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
};

}
