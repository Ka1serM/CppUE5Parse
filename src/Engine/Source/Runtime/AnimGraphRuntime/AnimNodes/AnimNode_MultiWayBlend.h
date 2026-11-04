#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/InputScaleBias.h"

#include "AnimNode_MultiWayBlend.generated.h"

namespace ue5parse::runtime {

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
