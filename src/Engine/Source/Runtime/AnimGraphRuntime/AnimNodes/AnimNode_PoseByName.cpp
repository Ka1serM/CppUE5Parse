#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_PoseByName.h"

#include "Engine/Source/Runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_PoseByName::RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) {
    PoseExtractContext.PoseCurves.Reset();
    if (InPoseAsset == nullptr)
        return;
    const TArray<FName>& PoseNames = InPoseAsset->GetPoseFNames();
    const int32 PoseIndex = InPoseAsset->GetPoseIndexByName(PoseName);
    if (PoseIndex != INDEX_NONE)
        PoseExtractContext.PoseCurves.Add(FPoseCurve(PoseIndex, PoseNames[PoseIndex], 0.f));
}

void FAnimNode_PoseByName::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    FAnimNode_PoseHandler::UpdateAssetPlayer(Context);
    if (CurrentPoseName == PoseName)
        return;
    RebuildPoseList(Context.AnimInstanceProxy->GetRequiredBones(), PoseAsset);
    CurrentPoseName = PoseName;
}

void FAnimNode_PoseByName::Evaluate_AnyThread(FPoseContext& Output) {
    if (CurrentPoseAsset == nullptr || PoseExtractContext.PoseCurves.Num() == 0 || CurrentPoseAsset->GetSkeleton() == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    PoseExtractContext.PoseCurves[0].Value = PoseWeight;
    FAnimationPoseData OutputAnimationPoseData = Output.GetAnimationPoseData();
    CurrentPoseAsset->GetAnimationPose(OutputAnimationPoseData, PoseExtractContext);
}

}
