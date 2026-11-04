#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/BonePose.h"

#include "AnimNode_LinkedInputPose.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNode_LinkedInputPose : public FAnimNode_Base {
    GENERATED_BODY()
public:
    static const FName DefaultInputPoseName;

    UPROPERTY()
    FName Name = DefaultInputPoseName;
    UPROPERTY()
    FName Graph;
    UPROPERTY()
    FPoseLink InputPose;
    int32 OuterGraphNodeIndex = INDEX_NONE;
    FCompactPose CachedInputPose;
    FBlendedCurve CachedInputCurve;
    bool bIsCachedInputPoseInitialized = false;

    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    void DynamicLink(FAnimInstanceProxy* InInputProxy, FPoseLinkBase* InPoseLink, int32 InOuterGraphNodeIndex);
    void DynamicUnlink();

private:
    FAnimInstanceProxy* InputProxy = nullptr;
};

}
