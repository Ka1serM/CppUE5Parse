#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_MakeDynamicAdditive.generated.h"

namespace ue5parse::runtime {

// The difference of a pose from a base pose, as an additive pose (UE's AnimNodes/AnimNode_MakeDynamicAdditive.h).
USTRUCT()
struct FAnimNode_MakeDynamicAdditive : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink Base;
    UPROPERTY()
    FPoseLink Additive;
    UPROPERTY()
    bool bMeshSpaceAdditive = false;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
};

}
