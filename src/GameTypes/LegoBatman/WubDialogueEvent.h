#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "WubDialogueEvent.generated.h"

namespace ue5parse::runtime {

//! One step of a dialogue sequence; the other steps are indices into the steps of the event, and -1 is an empty step.
USTRUCT()
struct FWubDialogueStep {
    GENERATED_BODY()
    UPROPERTY()
    uint8 Type = 0;
    UPROPERTY()
    TArray<FInstancedStruct> DialogueTriggerConditions;
    UPROPERTY()
    FName Character;
    UPROPERTY()
    FName WemName;
    UPROPERTY()
    TObjectPtr<UObject> AudioEvent = nullptr;
    UPROPERTY()
    FName AudioTag;
    UPROPERTY()
    FName EventId;
    UPROPERTY()
    float DelayInSeconds = 0;
    UPROPERTY()
    float TriggerChance = 0;
    UPROPERTY()
    FName AssociatedActor;
    UPROPERTY()
    int32 AdditionalSequence = -1;
    UPROPERTY()
    TArray<int32> InnerSequence;
};

//! LEGO Batman: Legacy of the Dark Knight saves a tree of dialogue steps after the properties.
UCLASS()
class UWubDialogueEvent : public UObject {
    GENERATED_BODY()
public:
    explicit UWubDialogueEvent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    //! The first step is the sequence of the event.
    UPROPERTY()
    TArray<FWubDialogueStep> Steps;
    UPROPERTY()
    TArray<FName> Wems;

private:
    int32 SerializeStep(FArchive& Ar, uint8 Type, int32 Depth);
    int32 SerializeTypedStep(FArchive& Ar, int32 Depth);
};

}
