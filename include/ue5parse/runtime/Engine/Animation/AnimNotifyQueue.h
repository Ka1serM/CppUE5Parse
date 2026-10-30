#pragma once

#include "ue5parse/runtime/Core/Containers/Map.h"
#include "ue5parse/runtime/Core/Math/RandomStream.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"

#include "AnimNotifyQueue.generated.h"

// The notifies an update collects (UE's Animation/AnimNotifyQueue.h): the players and montages add them with their
// weight, the instance triggers them after the evaluation.
namespace ue5parse::runtime {

class FAnimInstanceProxy;
class USkeletalMeshComponent;
struct FAnimTickRecord;

// A notify of an asset and the asset it came from. A reference without a source has no notify.
USTRUCT()
struct FAnimNotifyEventReference {
    GENERATED_BODY()
public:
    FAnimNotifyEventReference() = default;
    FAnimNotifyEventReference(const FAnimNotifyEvent* InNotify, const UObject* InNotifySource)
        : Notify(InNotify), NotifySource(const_cast<UObject*>(InNotifySource)) {}

    const FAnimNotifyEvent* GetNotify() const { return NotifySource ? Notify : nullptr; }
    void SetNotify(const FAnimNotifyEvent* InNotify) {
        if (NotifySource) Notify = InNotify;
    }
    const UObject* GetMirrorDataTable() const { return MirrorTable; }
    friend bool operator==(const FAnimNotifyEventReference& Lhs, const FAnimNotifyEventReference& Rhs) {
        return Lhs.Notify == Rhs.Notify || (Lhs.Notify && Rhs.Notify && *Lhs.Notify == *Rhs.Notify);
    }
    friend bool operator==(const FAnimNotifyEventReference& Lhs, const FAnimNotifyEvent& Rhs) {
        return Lhs.NotifySource && Lhs.Notify && *Lhs.Notify == Rhs;
    }
    // The time of the player that collected the notify.
    void GatherTickRecordData(const FAnimTickRecord& InTickRecord);
    const UObject* GetSourceObject() const { return NotifySource; }
    float GetCurrentAnimationTime() const { return CurrentAnimTime; }

private:
    const FAnimNotifyEvent* Notify = nullptr;
    UPROPERTY(transient)
    TObjectPtr<UObject> MirrorTable = nullptr;
    UPROPERTY(transient)
    TObjectPtr<UObject> NotifySource = nullptr;
    float CurrentAnimTime = 0.0f;
};

USTRUCT()
struct FAnimNotifyArray {
    GENERATED_BODY()
public:
    UPROPERTY(transient)
    TArray<FAnimNotifyEventReference> Notifies;
};

// The notifies one player collects, and its tick record.
struct FAnimNotifyContext {
    FAnimNotifyContext() = default;
    explicit FAnimNotifyContext(const FAnimTickRecord& InTickRecord) : TickRecord(&InTickRecord) {}

    const FAnimTickRecord* TickRecord = nullptr;
    TArray<FAnimNotifyEventReference> ActiveNotifies;
};

USTRUCT()
struct FAnimNotifyQueue {
    GENERATED_BODY()
public:
    FAnimNotifyQueue() { RandomStream.Initialize(0x05629063); }

    bool PassesFiltering(const FAnimNotifyEvent* Notify) const;
    bool PassesChanceOfTriggering(const FAnimNotifyEvent* Event) const;
    void AddAnimNotify(const FAnimNotifyEvent* Notify, const UObject* NotifySource);
    // A follower's notifies count only when they trigger on followers; each must pass the weight threshold, the LOD
    // filter and its chance.
    void AddAnimNotifies(bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, const float InstanceWeight);
    // The notifies of a montage's slots, kept until the update knows which slots are relevant.
    void AddAnimNotifies(bool bSrcIsLeader, const TMap<FName, TArray<FAnimNotifyEventReference>>& NewNotifies, const float InstanceWeight);
    void AddAnimNotifies(const TArray<FAnimNotifyEventReference>& NewNotifies, const float InstanceWeight) {
        AddAnimNotifies(true, NewNotifies, InstanceWeight);
    }
    void AddAnimNotifies(const TMap<FName, TArray<FAnimNotifyEventReference>>& NewNotifies, const float InstanceWeight) {
        AddAnimNotifies(true, NewNotifies, InstanceWeight);
    }
    void Reset(USkeletalMeshComponent* Component);
    void Append(const FAnimNotifyQueue& Queue);
    // The slot notifies of the slots that the update found relevant.
    void ApplyMontageNotifies(const FAnimInstanceProxy& Proxy);

    int32 PredictedLODLevel = -1;
    FRandomStream RandomStream;
    UPROPERTY(transient)
    TArray<FAnimNotifyEventReference> AnimNotifies;
    UPROPERTY(transient)
    TMap<FName, FAnimNotifyArray> UnfilteredMontageAnimNotifies;

private:
    void AddAnimNotifiesToDest(bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, TArray<FAnimNotifyEventReference>& DestArray,
                               const float InstanceWeight);
    void AddAnimNotifiesToDestNoFiltering(const TArray<FAnimNotifyEventReference>& NewNotifies, TArray<FAnimNotifyEventReference>& DestArray) const;
};

}
