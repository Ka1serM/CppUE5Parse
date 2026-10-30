#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"

namespace ue5parse::runtime {

// UE's FAnimSyncParams: the sync group a tick record joins and its role there; NAME_None ticks it alone.
struct FAnimSyncParams {
    FName GroupName;
    EAnimGroupRole::Type Role = EAnimGroupRole::CanBeLeader;
    EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;
};

// UE's FAnimGroupInstance: the records of one sync group in a frame. The record with the highest leader score leads.
struct FAnimGroupInstance {
    TArray<FAnimTickRecord> ActivePlayers;
    int32 GroupLeaderIndex = INDEX_NONE;
    float PreviousAnimLengthRatio = 0;
    float AnimLengthRatio = 0;

    // Scores the last added record: its weight when it can lead, 2 when it always leads, else 0.
    void TestTickRecordForLeadership(EAnimGroupRole::Type MembershipType);
    // Sorts the records by leader score, highest first; equal scores keep their update order.
    void Prepare();
};

// UE's FAnimSync (Animation/AnimSync.h): the tick records of one instance's update, and their advance after it.
class FAnimSync {
public:
    void Reset();
    void AddTickRecord(const FAnimTickRecord& InTickRecord, const FAnimSyncParams& InSyncParams);
    // Each group's leader advances by the delta time, then its followers match its position; ungrouped records advance
    // alone.
    void TickAssetPlayerInstances(struct FAnimNotifyQueue& NotifyQueue, float InDeltaSeconds);
    const FAnimGroupInstance* GetSyncGroupInstance(FName InSyncGroupName) const { return SyncGroupMap.Find(InSyncGroupName); }

private:
    TMap<FName, FAnimGroupInstance> SyncGroupMap;
    TArray<FAnimTickRecord> UngroupedActivePlayers;
};

}
