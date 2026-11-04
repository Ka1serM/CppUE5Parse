#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_Root.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_Root : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink Result;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override { Result.Initialize(Context); }
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override { Result.CacheBones(Context); }
    void Update_AnyThread(const FAnimationUpdateContext& Context) override { Result.Update(Context); }
    void Evaluate_AnyThread(FPoseContext& Output) override { Result.Evaluate(Output); }
    FName GetName() const { return GET_ANIM_NODE_DATA(FName, Name); }
    FName GetGroup() const { return GET_ANIM_NODE_DATA(FName, Group); }
};

}
