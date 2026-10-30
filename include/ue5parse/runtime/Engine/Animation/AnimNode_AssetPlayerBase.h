#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerRelevancyBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

#include "AnimNode_AssetPlayerBase.generated.h"

namespace ue5parse::runtime {

class UAnimSequenceBase;

// The base of the nodes that play an asset (UE's Animation/AnimNode_AssetPlayerBase.h). Its update adds a tick record;
// the instance's FAnimSync then advances InternalTimeAccumulator, alone or with the node's sync group.
USTRUCT()
struct FAnimNode_AssetPlayerBase : public FAnimNode_AssetPlayerRelevancyBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    float BlendWeight = 0;
    UPROPERTY()
    float InternalTimeAccumulator = 0;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    // The update of the player itself, after the base caches the weight.
    virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) {}

    float GetAccumulatedTime() const override { return InternalTimeAccumulator; }
    float GetCachedBlendWeight() const override { return BlendWeight; }
    void ClearCachedBlendWeight() override { BlendWeight = 0; }
    virtual FName GetGroupName() const { return FName(); }
    virtual EAnimGroupRole::Type GetGroupRole() const { return EAnimGroupRole::CanBeLeader; }
    virtual EAnimSyncMethod GetGroupMethod() const { return EAnimSyncMethod::DoNotSync; }

protected:
    // Adds the tick record of this frame. A transition role joins its group only once the node has had full weight.
    void CreateTickRecordForNode(const FAnimationUpdateContext& Context, const UAnimationAsset* Asset, bool bLooping, float PlayRate,
                                 bool bIsEvaluator);
    // Adds a tick record that the player filled (a blend space's), with the node's delta time record and sync group.
    void AddTickRecord(const FAnimationUpdateContext& Context, FAnimTickRecord& TickRecord);

    FDeltaTimeRecord DeltaTimeRecord;
    bool bHasBeenFullWeight = false;
};

}
