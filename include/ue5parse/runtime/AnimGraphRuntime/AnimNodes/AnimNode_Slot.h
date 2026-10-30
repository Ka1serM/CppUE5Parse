#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"

#include "AnimNode_Slot.generated.h"

namespace ue5parse::runtime {

// Plays the anim instance's montages of a slot over the source pose (UE's AnimNodes/AnimNode_Slot.h). The source is
// not updated while the montages cover it fully, unless bAlwaysUpdateSourcePose is set.
USTRUCT()
struct FAnimNode_Slot : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink Source;
    UPROPERTY()
    FName SlotName = FAnimSlotGroup::DefaultSlotName;
    UPROPERTY()
    bool bAlwaysUpdateSourcePose = false;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

private:
    FSlotNodeWeightInfo WeightData;
    FGraphTraversalCounter SlotNodeInitializationCounter;
};

}
