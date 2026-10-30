#include "ue5parse/runtime/Engine/Animation/AnimNodeSpaceConversions.h"

namespace ue5parse::runtime {

void FAnimNode_ConvertComponentToLocalSpace::Evaluate_AnyThread(FPoseContext& Output) {
    FComponentSpacePoseContext InputCSPose(Output.AnimInstanceProxy);
    ComponentPose.EvaluateComponentSpace(InputCSPose);
    FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(std::move(InputCSPose.Pose), Output.Pose);
    Output.Curve.CopyFrom(InputCSPose.Curve);
}

void FAnimNode_ConvertLocalToComponentSpace::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) {
    FPoseContext SourcePose(Output.AnimInstanceProxy);
    LocalPose.Evaluate(SourcePose);
    Output.Pose.InitPose(std::move(SourcePose.Pose));
    Output.Curve.CopyFrom(SourcePose.Curve);
}

}
