#include "ue5parse/runtime/Engine/Animation/AnimSync.h"

#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"

#include <algorithm>

namespace ue5parse::runtime {

namespace {
// UE's LEADERSCORE_ALWAYSLEADER: above any weight.
constexpr float LeaderScoreAlwaysLeader = 2.0f;
}

void FAnimGroupInstance::TestTickRecordForLeadership(EAnimGroupRole::Type MembershipType) {
    FAnimTickRecord& Candidate = ActivePlayers.Last();
    switch (MembershipType) {
        case EAnimGroupRole::CanBeLeader:
        case EAnimGroupRole::TransitionLeader: Candidate.LeaderScore = Candidate.EffectiveBlendWeight; break;
        case EAnimGroupRole::AlwaysLeader: Candidate.LeaderScore = LeaderScoreAlwaysLeader; break;
        default: break;
    }
}

void FAnimGroupInstance::Prepare() {
    std::stable_sort(ActivePlayers.begin(), ActivePlayers.end(),
                     [](const FAnimTickRecord& A, const FAnimTickRecord& B) { return A.LeaderScore > B.LeaderScore; });
}

void FAnimSync::Reset() {
    SyncGroupMap.Empty();
    UngroupedActivePlayers.Empty();
}

void FAnimSync::AddTickRecord(const FAnimTickRecord& InTickRecord, const FAnimSyncParams& InSyncParams) {
    if (InSyncParams.GroupName.isNone()) {
        UngroupedActivePlayers.Add(InTickRecord);
        return;
    }
    FAnimGroupInstance& SyncGroupInstance = SyncGroupMap.FindOrAdd(InSyncParams.GroupName);
    SyncGroupInstance.ActivePlayers.Add(InTickRecord);
    SyncGroupInstance.TestTickRecordForLeadership(InSyncParams.Role);
}

void FAnimSync::TickAssetPlayerInstances(FAnimNotifyQueue& NotifyQueue, float InDeltaSeconds) {
    for (auto& [GroupName, SyncGroup] : SyncGroupMap) {
        if (SyncGroup.ActivePlayers.Num() == 0) continue;
        SyncGroup.Prepare();
        FAnimAssetTickContext TickContext(InDeltaSeconds, SyncGroup.ActivePlayers.Num() == 1);
        FAnimTickRecord& GroupLeader = SyncGroup.ActivePlayers[0];
        GroupLeader.SourceAsset->TickAssetPlayer(GroupLeader, NotifyQueue, TickContext);
        SyncGroup.PreviousAnimLengthRatio = TickContext.GetPreviousAnimationPositionRatio();
        SyncGroup.AnimLengthRatio = TickContext.GetAnimationPositionRatio();
        SyncGroup.GroupLeaderIndex = 0;
        TickContext.ConvertToFollower();
        for (int32 TickIndex = 1; TickIndex < SyncGroup.ActivePlayers.Num(); ++TickIndex) {
            FAnimTickRecord& AssetPlayer = SyncGroup.ActivePlayers[TickIndex];
            AssetPlayer.SourceAsset->TickAssetPlayer(AssetPlayer, NotifyQueue, TickContext);
        }
    }
    for (FAnimTickRecord& AssetPlayerToTick : UngroupedActivePlayers) {
        FAnimAssetTickContext TickContext(InDeltaSeconds, true);
        AssetPlayerToTick.SourceAsset->TickAssetPlayer(AssetPlayerToTick, NotifyQueue, TickContext);
    }
}

}
