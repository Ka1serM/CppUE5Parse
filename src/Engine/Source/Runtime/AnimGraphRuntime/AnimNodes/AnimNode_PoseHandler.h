#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNode_AssetPlayerBase.h"
#include "Engine/Source/Runtime/Engine/Animation/PoseAsset.h"

#include "AnimNode_PoseHandler.generated.h"

namespace ue5parse::runtime {

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
    virtual void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset);
    void CacheBoneBlendWeights(FAnimInstanceProxy* InstanceProxy);

    const UPoseAsset* CurrentPoseAsset = nullptr;
    FAnimExtractContext PoseExtractContext;
    TArray<float> BoneBlendWeights;

private:
    void UpdatePoseAssetProperty(FAnimInstanceProxy* InstanceProxy);
};

}
