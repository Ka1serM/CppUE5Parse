#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_PoseHandler.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_PoseHandler::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
    UpdatePoseAssetProperty(Context.AnimInstanceProxy);
}

void FAnimNode_PoseHandler::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    FAnimNode_AssetPlayerBase::CacheBones_AnyThread(Context);
    CacheBoneBlendWeights(Context.AnimInstanceProxy);
}

void FAnimNode_PoseHandler::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    if (CurrentPoseAsset == nullptr || CurrentPoseAsset != PoseAsset) UpdatePoseAssetProperty(Context.AnimInstanceProxy);
}

void FAnimNode_PoseHandler::RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) {
    PoseExtractContext.PoseCurves.Reset();
    const TArray<FName>& PoseNames = InPoseAsset->GetPoseFNames();
    for (int32 PoseIndex = 0; PoseIndex < PoseNames.Num(); PoseIndex++) PoseExtractContext.PoseCurves.Add(FPoseCurve(PoseIndex, PoseNames[PoseIndex], 0.f));
}

void FAnimNode_PoseHandler::CacheBoneBlendWeights(FAnimInstanceProxy* InstanceProxy) {
    BoneBlendWeights.Reset();
    const FBoneContainer& BoneContainer = InstanceProxy->GetRequiredBones();
    if (CurrentPoseAsset == nullptr || !BoneContainer.IsValid()) {
        PoseExtractContext.PoseCurves.Reset();
        return;
    }
    BoneBlendWeights.SetNum(BoneContainer.GetBoneIndicesArray().Num());
    for (const FName& TrackName : CurrentPoseAsset->GetTrackNames()) {
        const FCompactPoseBoneIndex CompactBoneIndex =
            BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneContainer.GetPoseBoneIndexForBoneName(TrackName)));
        if (CompactBoneIndex != INDEX_NONE) BoneBlendWeights[CompactBoneIndex.GetInt()] = 1.f;
    }
    RebuildPoseList(BoneContainer, CurrentPoseAsset);
}

void FAnimNode_PoseHandler::UpdatePoseAssetProperty(FAnimInstanceProxy* InstanceProxy) {
    CurrentPoseAsset = PoseAsset;
    CacheBoneBlendWeights(InstanceProxy);
}

}
