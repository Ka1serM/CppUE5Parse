#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/AnimNodes/AnimNode_PoseHandler.h"

#include "AnimNode_PoseByName.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_PoseByName : public FAnimNode_PoseHandler {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName PoseName;
    UPROPERTY()
    float PoseWeight = 1.f;

    void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

private:
    void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) override;

    FName CurrentPoseName;
};

}
