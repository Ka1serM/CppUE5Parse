#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

namespace ue5parse::runtime {

void FAnimNotifyEventReference::GatherTickRecordData(const FAnimTickRecord& InTickRecord) {
    CurrentAnimTime = InTickRecord.TimeAccumulator != nullptr ? *InTickRecord.TimeAccumulator : 0.0f;
}

bool FAnimNotifyQueue::PassesFiltering(const FAnimNotifyEvent* Notify) const {
    if (Notify->NotifyFilterType == ENotifyFilterType::LOD) return Notify->NotifyFilterLOD > PredictedLODLevel;
    return true;
}

bool FAnimNotifyQueue::PassesChanceOfTriggering(const FAnimNotifyEvent* Event) const {
    return Event->NotifyStateClass ? true : RandomStream.FRandRange(0.f, 1.f) < Event->NotifyTriggerChance;
}

void FAnimNotifyQueue::AddAnimNotifiesToDest(bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies,
                                             TArray<FAnimNotifyEventReference>& DestArray, const float InstanceWeight) {
    for (const FAnimNotifyEventReference& NotifyRef : NewNotifies) {
        const FAnimNotifyEvent* Notify = NotifyRef.GetNotify();
        if (Notify == nullptr || (!bSrcIsLeader && !Notify->bTriggerOnFollower)) continue;
        // Not a dedicated server, so bTriggerOnDedicatedServer does not filter.
        if (Notify->TriggerWeightThreshold > InstanceWeight || !PassesFiltering(Notify) || !PassesChanceOfTriggering(Notify)) continue;
        // A state is added once: it stays active over several frames.
        if (Notify->NotifyStateClass) DestArray.AddUnique(NotifyRef);
        else DestArray.Add(NotifyRef);
    }
}

void FAnimNotifyQueue::AddAnimNotifiesToDestNoFiltering(const TArray<FAnimNotifyEventReference>& NewNotifies,
                                                        TArray<FAnimNotifyEventReference>& DestArray) const {
    for (const FAnimNotifyEventReference& NotifyRef : NewNotifies) {
        const FAnimNotifyEvent* Notify = NotifyRef.GetNotify();
        if (Notify == nullptr) continue;
        if (Notify->NotifyStateClass) DestArray.AddUnique(NotifyRef);
        else DestArray.Add(NotifyRef);
    }
}

void FAnimNotifyQueue::AddAnimNotify(const FAnimNotifyEvent* Notify, const UObject* NotifySource) {
    AnimNotifies.Add(FAnimNotifyEventReference(Notify, NotifySource));
}

void FAnimNotifyQueue::AddAnimNotifies(bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, const float InstanceWeight) {
    AddAnimNotifiesToDest(bSrcIsLeader, NewNotifies, AnimNotifies, InstanceWeight);
}

void FAnimNotifyQueue::AddAnimNotifies(bool bSrcIsLeader, const TMap<FName, TArray<FAnimNotifyEventReference>>& NewNotifies, const float InstanceWeight) {
    for (const auto& Pair : NewNotifies) {
        TArray<FAnimNotifyEventReference>& Notifies = UnfilteredMontageAnimNotifies.FindOrAdd(Pair.Key).Notifies;
        AddAnimNotifiesToDest(bSrcIsLeader, Pair.Value, Notifies, InstanceWeight);
    }
}

void FAnimNotifyQueue::Reset(USkeletalMeshComponent* Component) {
    AnimNotifies.Reset();
    UnfilteredMontageAnimNotifies.Reset();
    PredictedLODLevel = Component ? Component->GetPredictedLODLevel() : -1;
}

void FAnimNotifyQueue::Append(const FAnimNotifyQueue& Queue) {
    AddAnimNotifiesToDestNoFiltering(Queue.AnimNotifies, AnimNotifies);
    for (const auto& Pair : Queue.UnfilteredMontageAnimNotifies) {
        TArray<FAnimNotifyEventReference>& Notifies = UnfilteredMontageAnimNotifies.FindOrAdd(Pair.Key).Notifies;
        AddAnimNotifiesToDestNoFiltering(Pair.Value.Notifies, Notifies);
    }
}

void FAnimNotifyQueue::ApplyMontageNotifies(const FAnimInstanceProxy& Proxy) {
    for (const auto& Pair : UnfilteredMontageAnimNotifies)
        if (Proxy.IsSlotNodeRelevantForNotifies(Pair.Key)) AddAnimNotifiesToDestNoFiltering(Pair.Value.Notifies, AnimNotifies);
    UnfilteredMontageAnimNotifies.Reset();
}

}
