#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimLinkableElement.h"

#include "AnimTypes.generated.h"

namespace ue5parse::runtime {

class UAnimNotify;
class UAnimNotifyState;

namespace EMontageNotifyTickType {
enum Type : int32 { Queued, BranchingPoint };
}

namespace ENotifyFilterType {
enum Type : int32 { NoFiltering, LOD };
}

// FAnimNotifyEvent: a notify, or a notify state when NotifyStateClass is set (UE's Animation/AnimTypes.h). The linked
// time is the start; EndLink is the end of a state.
USTRUCT()
struct FAnimNotifyEvent : public FAnimLinkableElement {
    GENERATED_BODY()
    UPROPERTY()
    float TriggerTimeOffset = 0;
    UPROPERTY()
    float EndTriggerTimeOffset = 0;
    UPROPERTY()
    float TriggerWeightThreshold = ZERO_ANIMWEIGHT_THRESH;
    UPROPERTY()
    FName NotifyName;
    UPROPERTY()
    UAnimNotify* Notify = nullptr;
    UPROPERTY()
    UAnimNotifyState* NotifyStateClass = nullptr;
    UPROPERTY()
    float Duration = 0;
    UPROPERTY()
    FAnimLinkableElement EndLink;
    UPROPERTY()
    bool bConvertedFromBranchingPoint = false;
    UPROPERTY()
    TEnumAsByte<EMontageNotifyTickType::Type> MontageTickType = EMontageNotifyTickType::Queued;
    UPROPERTY()
    float NotifyTriggerChance = 1;
    UPROPERTY()
    TEnumAsByte<ENotifyFilterType::Type> NotifyFilterType = ENotifyFilterType::NoFiltering;
    UPROPERTY()
    int32 NotifyFilterLOD = 0;
    UPROPERTY()
    bool bTriggerOnDedicatedServer = true;
    UPROPERTY()
    bool bTriggerOnFollower = false;
    UPROPERTY()
    int32 TrackIndex = 0;

    float GetTriggerTime() const { return GetTime() + TriggerTimeOffset; }
    float GetEndTriggerTime() const;
    float GetDuration() const { return NotifyStateClass ? EndLink.GetTime() - GetTime() : 0.0f; }
    bool IsBranchingPoint() const { return MontageTickType == EMontageNotifyTickType::BranchingPoint; }
};

}
