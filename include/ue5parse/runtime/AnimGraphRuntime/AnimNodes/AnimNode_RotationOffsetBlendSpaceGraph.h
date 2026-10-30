#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpaceGraphBase.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_RotationOffsetBlendSpaceGraph.generated.h"

namespace ue5parse::runtime {

// An aim offset whose samples are graphs (UE's AnimNodes/AnimNode_RotationOffsetBlendSpaceGraph.h).
USTRUCT()
struct FAnimNode_RotationOffsetBlendSpaceGraph : public FAnimNode_BlendSpaceGraphBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink BasePose;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;
    UPROPERTY()
    float Alpha = 1.0f;
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

    FName GetGroupName() const { return GroupName; }
    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
};

}
