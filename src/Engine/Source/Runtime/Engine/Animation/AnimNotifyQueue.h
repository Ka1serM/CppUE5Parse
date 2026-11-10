#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Math/RandomStream.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimTypes.h"

#include "AnimNotifyQueue.generated.h"

namespace ue5parse::runtime {

class FAnimInstanceProxy;
class USkeletalMeshComponent;
struct FAnimTickRecord;

USTRUCT()
struct FAnimNotifyEventReference {
    GENERATED_BODY()
public:
    FAnimNotifyEventReference() = default;
    FAnimNotifyEventReference(const FAnimNotifyEvent* InNotify, const UObject* InNotifySource)
        : Notify(InNotify), NotifySource(const_cast<UObject*>(InNotifySource)) {}

    const FAnimNotifyEvent* GetNotify() const { return NotifySource ? Notify : nullptr; }
    void SetNotify(const FAnimNotifyEvent* InNotify) {
        if (NotifySource)
            Notify = InNotify;
    }
    friend bool operator==(const FAnimNotifyEventReference& Lhs, const FAnimNotifyEventReference& Rhs) {
        return Lhs.Notify == Rhs.Notify || (Lhs.Notify && Rhs.Notify && *Lhs.Notify == *Rhs.Notify);
    }
    friend bool operator==(const FAnimNotifyEventReference& Lhs, const FAnimNotifyEvent& Rhs) { return Lhs.NotifySource && Lhs.Notify && *Lhs.Notify == Rhs; }
    void GatherTickRecordData(const FAnimTickRecord& InTickRecord);

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
    void AddAnimNotifies(bool bSrcIsLeader, const TArray<FAnimNotifyEventReference>& NewNotifies, const float InstanceWeight);
    void AddAnimNotifies(bool bSrcIsLeader, const TMap<FName, TArray<FAnimNotifyEventReference>>& NewNotifies, const float InstanceWeight);
    void AddAnimNotifies(const TArray<FAnimNotifyEventReference>& NewNotifies, const float InstanceWeight) {
        AddAnimNotifies(true, NewNotifies, InstanceWeight);
    }
    void AddAnimNotifies(const TMap<FName, TArray<FAnimNotifyEventReference>>& NewNotifies, const float InstanceWeight) {
        AddAnimNotifies(true, NewNotifies, InstanceWeight);
    }
    void Reset(USkeletalMeshComponent* Component);
    void Append(const FAnimNotifyQueue& Queue);
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
