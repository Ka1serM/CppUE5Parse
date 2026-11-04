#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_MakeDynamicAdditive.generated.h"

namespace ue5parse::runtime {

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
