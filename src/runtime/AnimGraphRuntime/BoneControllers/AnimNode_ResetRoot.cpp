#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_ResetRoot.h"

namespace ue5parse::runtime {

void FAnimNode_ResetRoot::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    const FCompactPoseBoneIndex RootBoneIndex(0);
    OutBoneTransforms.Add(FBoneTransform(RootBoneIndex, BoneContainer.GetRefPoseTransform(RootBoneIndex)));
    for (const FCompactPoseBoneIndex Child : RootChildren) OutBoneTransforms.Add(FBoneTransform(Child, Output.Pose.GetComponentSpaceTransform(Child)));
    OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

void FAnimNode_ResetRoot::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    RootChildren.Reset();
    for (int32 Index = 0; Index < RequiredBones.GetCompactPoseNumBones(); Index++) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        if (RequiredBones.GetParentBoneIndex(BoneIndex) == 0) RootChildren.Add(BoneIndex);
    }
}

}
