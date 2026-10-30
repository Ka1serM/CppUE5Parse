#include "ue5parse/runtime/Engine/Animation/AnimNode_AssetPlayerBase.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_AssetPlayerBase::Initialize_AnyThread(const FAnimationInitializeContext& Context) { bHasBeenFullWeight = false; }

void FAnimNode_AssetPlayerBase::Update_AnyThread(const FAnimationUpdateContext& Context) {
    BlendWeight = Context.GetFinalBlendWeight();
    bHasBeenFullWeight = bHasBeenFullWeight || FAnimWeight::IsFullWeight(BlendWeight);
    UpdateAssetPlayer(Context);
}

void FAnimNode_AssetPlayerBase::CreateTickRecordForNode(const FAnimationUpdateContext& Context, const UAnimationAsset* Asset, bool bLooping,
                                                       float PlayRate, bool bIsEvaluator) {
    FAnimTickRecord TickRecord(Asset, bLooping, PlayRate, bIsEvaluator, Context.GetFinalBlendWeight(), InternalTimeAccumulator);
    AddTickRecord(Context, TickRecord);
}

void FAnimNode_AssetPlayerBase::AddTickRecord(const FAnimationUpdateContext& Context, FAnimTickRecord& TickRecord) {
    const EAnimGroupRole::Type SyncGroupRole = GetGroupRole();
    const bool bTransitionRole = SyncGroupRole == EAnimGroupRole::TransitionLeader || SyncGroupRole == EAnimGroupRole::TransitionFollower;
    FAnimSyncParams SyncParams{bTransitionRole && !bHasBeenFullWeight ? FName() : GetGroupName(), SyncGroupRole, GetGroupMethod()};
    if (SyncParams.GroupName.isNone() && SyncParams.Method == EAnimSyncMethod::SyncGroup) SyncParams.Method = EAnimSyncMethod::DoNotSync;
    TickRecord.RootMotionWeightModifier = Context.GetRootMotionWeightModifier();
    TickRecord.DeltaTimeRecord = &DeltaTimeRecord;
    Context.AnimInstanceProxy->GetSync().AddTickRecord(TickRecord, SyncParams);
}

}
