#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerBase.h"
#include "ue5parse/runtime/Engine/Animation/PoseAsset.h"

#include "AnimNode_PoseHandler.generated.h"

namespace ue5parse::runtime {

// The base of the nodes that evaluate a pose asset (UE's AnimNodes/AnimNode_PoseHandler.h). It keeps a pose curve per
// pose of the current asset and the weight of each bone with a track in it.
USTRUCT()
struct FAnimNode_PoseHandler : public FAnimNode_AssetPlayerBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPoseAsset> PoseAsset = nullptr;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    UAnimationAsset* GetAnimAsset() const override { return PoseAsset; }

protected:
    // Makes the pose curves of the asset, after the asset or the bones change.
    virtual void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset);
    void CacheBoneBlendWeights(FAnimInstanceProxy* InstanceProxy);

    const UPoseAsset* CurrentPoseAsset = nullptr;
    FAnimExtractContext PoseExtractContext;
    // Per compact pose bone: 1 for a bone with a track in the asset, else 0.
    TArray<float> BoneBlendWeights;

private:
    void UpdatePoseAssetProperty(FAnimInstanceProxy* InstanceProxy);
};

}
