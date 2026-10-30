#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpacePlayer.h"

#include "AnimNode_BlendSpaceEvaluator.generated.h"

namespace ue5parse::runtime {

// A blend space at an input time instead of a played one (UE's AnimNodes/AnimNode_BlendSpaceEvaluator.h).
USTRUCT()
struct FAnimNode_BlendSpaceEvaluator : public FAnimNode_BlendSpacePlayer {
    GENERATED_BODY()
public:
    UPROPERTY()
    float NormalizedTime = 0.f;
    // Jump to the time (no notifies); else advance to it.
    UPROPERTY()
    bool bTeleportToNormalizedTime = true;

    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    float GetPlayRate() const override { return bTeleportToNormalizedTime ? 0.0f : 1.0f; }
    bool ShouldTeleportToTime() const override { return bTeleportToNormalizedTime; }
    bool IsEvaluator() const override { return true; }
};

}
