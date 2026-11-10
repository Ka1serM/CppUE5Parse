#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimLinkableElement.h"

#include "AnimTypes.generated.h"

namespace ue5parse::runtime {

class UAnimNotify;
class UAnimNotifyState;

enum EAdditiveAnimationType : int32 { AAT_None, AAT_LocalSpaceBase, AAT_RotationOffsetMeshSpace };

enum EBoneAxis : int { BA_X, BA_Y, BA_Z };

namespace EComponentType {

enum Type : int { None, TranslationX, TranslationY, TranslationZ, RotationX, RotationY, RotationZ, Scale, ScaleX, ScaleY, ScaleZ };

}

namespace EMontageNotifyTickType {

enum Type : int32 { Queued, BranchingPoint };

}

namespace ENotifyFilterType {

enum Type : int32 { NoFiltering, LOD };

}

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
    TObjectPtr<UAnimNotify> Notify = nullptr;
    UPROPERTY()
    TObjectPtr<UAnimNotifyState> NotifyStateClass = nullptr;
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
    bool IsBlueprintNotify() const { return Notify != nullptr || NotifyStateClass != nullptr; }
    FName GetNotifyEventName() const;
    bool operator==(const FAnimNotifyEvent& Other) const {
        return (Notify && Notify == Other.Notify) || (NotifyStateClass && NotifyStateClass == Other.NotifyStateClass)
               || (!IsBlueprintNotify() && NotifyName == Other.NotifyName);
    }
};

namespace ECurveBlendOption {

enum Type : int32 { Override, DoNotOverride, NormalizeByWeight, BlendByWeight, UseBasePose, UseMaxValue, UseMinValue };

}

USTRUCT()
struct FPerBoneBlendWeight {
    GENERATED_BODY()
    UPROPERTY()
    int32 SourceIndex = 0;
    UPROPERTY()
    float BlendWeight = 0;
};

struct FAnimWeight {
    static bool IsRelevant(float InWeight) { return InWeight > ZERO_ANIMWEIGHT_THRESH; }
    static bool IsFullWeight(float InWeight) { return InWeight >= 1.0f - ZERO_ANIMWEIGHT_THRESH; }
    static float GetSmallestRelevantWeight() { return 2.0f * ZERO_ANIMWEIGHT_THRESH; }
};

struct FGraphTraversalCounter {
    int32 Get() const { return Counter; }
    bool HasEverBeenUpdated() const { return Counter != INDEX_NONE; }
    void Increment() {
        Counter++;
        if (Counter == INDEX_NONE)
            Counter++;
    }
    void Reset() { Counter = INDEX_NONE; }
    void SynchronizeWith(const FGraphTraversalCounter& InMasterCounter) { Counter = InMasterCounter.Counter; }
    bool IsSynchronized_Counter(const FGraphTraversalCounter& InMasterCounter) const { return Counter == InMasterCounter.Counter; }
    bool WasSynchronizedCounter(const FGraphTraversalCounter& InMasterCounter) const {
        if (IsSynchronized_Counter(InMasterCounter))
            return true;
        FGraphTraversalCounter Next(*this);
        Next.Increment();
        return Next.IsSynchronized_Counter(InMasterCounter);
    }

private:
    int32 Counter = INDEX_NONE;
};

struct FSlotNodeWeightInfo {
    float SlotNodeWeight = 0;
    float SourceWeight = 0;
    float TotalNodeWeight = 0;
};

}
