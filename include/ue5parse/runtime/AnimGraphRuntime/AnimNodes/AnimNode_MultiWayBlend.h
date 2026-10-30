#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_MultiWayBlend.generated.h"

namespace ue5parse::runtime {

// Blends any number of poses by their alphas, normalized to a sum of 1 or not (UE's
// AnimNodes/AnimNode_MultiWayBlend.h).
USTRUCT()
struct FAnimNode_MultiWayBlend : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FPoseLink> Poses;
    UPROPERTY()
    TArray<float> DesiredAlphas;
    UPROPERTY()
    FInputScaleBias AlphaScaleBias;
    UPROPERTY()
    bool bAdditiveNode = false;
    UPROPERTY()
    bool bNormalizeAlpha = true;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
    float GetTotalAlpha() const;

private:
    void UpdateCachedAlphas();

    TArray<float> CachedAlphas;
};

}
