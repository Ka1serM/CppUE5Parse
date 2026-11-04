#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimationAsset.h"

namespace ue5parse::runtime {

struct FAnimSyncParams {
    FName GroupName;
    EAnimGroupRole::Type Role = EAnimGroupRole::CanBeLeader;
    EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;
};

struct FAnimGroupInstance {
    TArray<FAnimTickRecord> ActivePlayers;
    int32 GroupLeaderIndex = INDEX_NONE;
    float PreviousAnimLengthRatio = 0;
    float AnimLengthRatio = 0;

    void TestTickRecordForLeadership(EAnimGroupRole::Type MembershipType);
    void Prepare();
};

class FAnimSync {
public:
    void Reset();
    void AddTickRecord(const FAnimTickRecord& InTickRecord, const FAnimSyncParams& InSyncParams);
    void TickAssetPlayerInstances(struct FAnimNotifyQueue& NotifyQueue, float InDeltaSeconds);

private:
    TMap<FName, FAnimGroupInstance> SyncGroupMap;
    TArray<FAnimTickRecord> UngroupedActivePlayers;
};

}
