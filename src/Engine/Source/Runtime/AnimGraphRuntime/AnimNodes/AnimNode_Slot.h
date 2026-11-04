#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"

#include "AnimNode_Slot.generated.h"

namespace ue5parse::runtime {

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
