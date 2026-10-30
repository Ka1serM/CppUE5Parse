#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/BonePose.h"

#include "AnimNode_LinkedInputPose.generated.h"

namespace ue5parse::runtime {

// The input pose of a linked graph: the pose that the outer graph's linked node passes in (UE's
// Animation/AnimNode_LinkedInputPose.h).
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
    // The pose a post process instance takes in: the main instance's output, which the component pushes before the
    // evaluation (UE's CachedInputPose). Its bones can be allocated before they are set.
    FCompactPose CachedInputPose;
    FBlendedCurve CachedInputCurve;
    bool bIsCachedInputPoseInitialized = false;

    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

    // Runs the outer graph's pose link with the outer instance's proxy.
    void DynamicLink(FAnimInstanceProxy* InInputProxy, FPoseLinkBase* InPoseLink, int32 InOuterGraphNodeIndex);
    void DynamicUnlink();

private:
    FAnimInstanceProxy* InputProxy = nullptr;
};

}
