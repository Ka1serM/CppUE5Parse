#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpacePlayer.h"

#include "AnimNode_BlendSpaceEvaluator.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_BlendSpaceEvaluator : public FAnimNode_BlendSpacePlayer {
    GENERATED_BODY()
public:
    UPROPERTY()
    float NormalizedTime = 0.f;
    UPROPERTY()
    bool bTeleportToNormalizedTime = true;

    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    float GetPlayRate() const override { return bTeleportToNormalizedTime ? 0.0f : 1.0f; }
    bool ShouldTeleportToTime() const override { return bTeleportToNormalizedTime; }
    bool IsEvaluator() const override { return true; }
};

}
