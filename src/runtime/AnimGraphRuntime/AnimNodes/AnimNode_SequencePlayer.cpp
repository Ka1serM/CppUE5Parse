#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_SequencePlayer.h"

#include <algorithm>
#include <cmath>

namespace ue5parse::runtime {

void FAnimNode_SequencePlayer::Initialize_AnyThread() {
    const float Length = Sequence ? Sequence->GetPlayLength() : 0;
    InternalTimeAccumulator = std::clamp(StartPosition, 0.0f, Length);
}

void FAnimNode_SequencePlayer::Update_AnyThread(const FAnimationUpdateContext& Context) {
    if (!Sequence || Context.FinalBlendWeight <= 0) return;
    const float Length = Sequence->GetPlayLength();
    if (Length <= 0) {
        InternalTimeAccumulator = 0;
        return;
    }
    const float Time = InternalTimeAccumulator + Context.DeltaTime * PlayRate * Sequence->RateScale;
    if (bLoopAnimation) {
        InternalTimeAccumulator = std::fmod(Time, Length);
        if (InternalTimeAccumulator < 0) InternalTimeAccumulator += Length;
    } else {
        InternalTimeAccumulator = std::clamp(Time, 0.0f, Length);
    }
}

void FAnimNode_SequencePlayer::Evaluate_AnyThread(FPoseContext& Output) {
    if (!Sequence) {
        Output.ResetToRefPose();
        return;
    }
    FAnimationPoseData PoseData = Output.GetAnimationPoseData();
    Sequence->GetAnimationPose(PoseData, FAnimExtractContext(InternalTimeAccumulator, false, {}, bLoopAnimation));
}

}
