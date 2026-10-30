#include "ue5parse/runtime/Engine/Animation/AnimNode_LinkedInputPose.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

const FName FAnimNode_LinkedInputPose::DefaultInputPoseName("InPose");

void FAnimNode_LinkedInputPose::Update_AnyThread(const FAnimationUpdateContext& Context) {
    if (InputProxy == nullptr) return;
    const FAnimationUpdateContext InputContext = Context.WithOtherProxy(InputProxy);
    InputPose.Update(InputContext);
}

void FAnimNode_LinkedInputPose::Evaluate_AnyThread(FPoseContext& Output) {
    if (InputProxy != nullptr) {
        Output.Pose.SetBoneContainer(&InputProxy->GetRequiredBones());
        FPoseContext InputContext(InputProxy, Output.ExpectsAdditivePose());
        InputPose.Evaluate(InputContext);
        Output.Pose.MoveBonesFrom(InputContext.Pose);
        Output.Curve.CopyFrom(InputContext.Curve);
        return;
    }
    if (bIsCachedInputPoseInitialized && CachedInputPose.IsValid() && Output.Pose.GetNumBones() == CachedInputPose.GetNumBones()) {
        Output.Pose.CopyBonesFrom(CachedInputPose);
        Output.Curve.CopyFrom(CachedInputCurve);
        return;
    }
    Output.ResetToRefPose();
}

void FAnimNode_LinkedInputPose::DynamicLink(FAnimInstanceProxy* InInputProxy, FPoseLinkBase* InPoseLink, int32 InOuterGraphNodeIndex) {
    InputProxy = InInputProxy;
    InputPose.SetDynamicLinkNode(InPoseLink);
    OuterGraphNodeIndex = InOuterGraphNodeIndex;
}

void FAnimNode_LinkedInputPose::DynamicUnlink() {
    InputProxy = nullptr;
    InputPose.SetDynamicLinkNode(nullptr);
    OuterGraphNodeIndex = INDEX_NONE;
}

}
