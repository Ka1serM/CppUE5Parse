#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNode_AssetPlayerRelevancyBase.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimationAsset.h"

#include "AnimNode_AssetPlayerBase.generated.h"

namespace ue5parse::runtime {

class UAnimSequenceBase;

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
    virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) {}

    float GetAccumulatedTime() const override { return InternalTimeAccumulator; }
    float GetCachedBlendWeight() const override { return BlendWeight; }
    void ClearCachedBlendWeight() override { BlendWeight = 0; }
    virtual FName GetGroupName() const { return FName(); }
    virtual EAnimGroupRole::Type GetGroupRole() const { return EAnimGroupRole::CanBeLeader; }
    virtual EAnimSyncMethod GetGroupMethod() const { return EAnimSyncMethod::DoNotSync; }

protected:
    void CreateTickRecordForNode(const FAnimationUpdateContext& Context, const UAnimationAsset* Asset, bool bLooping, float PlayRate,
                                 bool bIsEvaluator);
    void AddTickRecord(const FAnimationUpdateContext& Context, FAnimTickRecord& TickRecord);

    FDeltaTimeRecord DeltaTimeRecord;
    bool bHasBeenFullWeight = false;
};

}
