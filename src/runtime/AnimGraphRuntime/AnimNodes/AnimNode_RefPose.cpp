#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_RefPose.h"

namespace ue5parse::runtime {

void FAnimNode_RefPose::Evaluate_AnyThread(FPoseContext& Output) {
    if (GetRefPoseType() == EIT_LocalSpace) Output.ResetToRefPose();
    else Output.ResetToAdditiveIdentity();
}

}
