#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendSpaceEvaluator.h"

namespace ue5parse::runtime {

void FAnimNode_BlendSpaceEvaluator::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    InternalTimeAccumulator = FMath::Clamp(NormalizedTime, 0.f, 1.f);
    UpdateInternal(Context);
}

}
