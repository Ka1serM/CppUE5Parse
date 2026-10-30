#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"

namespace ue5parse::runtime {

struct FAnimNode_SequencePlayer : FAnimNode_Base {
    UAnimSequence* Sequence = nullptr;
    float InternalTimeAccumulator = 0;
    float PlayRate = 1;
    float StartPosition = 0;
    bool bLoopAnimation = true;

    void Initialize_AnyThread() override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;
};

}
