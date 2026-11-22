#include "GameTypes/LegoBatman/WubDialogueEvent.h"

namespace ue5parse::runtime {

namespace {

constexpr int32 MaxStepDepth = 256;

void SkipFixedArray(FArchive& Ar, int32 ElementSize) {
    const int32 Count = Ar.ReadCount(ElementSize);
    Ar.SkipBytes(static_cast<int64>(Count) * ElementSize);
}

}

void UWubDialogueEvent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (!Ar.IsError())
        SerializeStep(Ar, 2, 0);
}

int32 UWubDialogueEvent::SerializeTypedStep(FArchive& Ar, int32 Depth) {
    const uint8 Type = Ar.Read<uint8>();
    if (Ar.IsError() || Type == 0)
        return -1;
    if (Type > 4) {
        Ar.SetError(ErrorCode::InvalidData, "UWubDialogueEvent.StepType");
        return -1;
    }
    return SerializeStep(Ar, Type, Depth + 1);
}

int32 UWubDialogueEvent::SerializeStep(FArchive& Ar, uint8 Type, int32 Depth) {
    if (Depth > MaxStepDepth) {
        Ar.SetError(ErrorCode::InvalidData, "UWubDialogueEvent.Depth");
        return -1;
    }
    const int32 Index = Steps.AddDefaulted();
    TArray<FInstancedStruct> Conditions;
    Ar.SkipBytes(1);
    const int32 ConditionCount = Ar.ReadCount(1);
    Conditions.Empty(ConditionCount);
    for (int32 Condition = 0; Condition < ConditionCount && !Ar.IsError(); Condition++)
        Conditions[Conditions.AddDefaulted()].Serialize(Ar);
    Ar.SkipBytes(1);
    Steps[Index].Type = Type;
    Steps[Index].DialogueTriggerConditions = std::move(Conditions);
    if (Ar.IsError())
        return Index;

    TArray<int32> Inner;
    switch (Type) {
    case 1: {
        FName Character;
        FName WemName;
        TObjectPtr<UObject> AudioEvent;
        FName AudioTag;
        Ar << Character << WemName;
        Ar.SkipBytes(4);
        SkipFixedArray(Ar, 10);
        Ar << AudioEvent;
        Ar.SkipBytes(18);
        Ar << AudioTag;
        Wems.AddUnique(WemName);
        Steps[Index].Character = Character;
        Steps[Index].WemName = WemName;
        Steps[Index].AudioEvent = AudioEvent;
        Steps[Index].AudioTag = AudioTag;
        return Index;
    }
    case 2: {
        FName EventId;
        float DelayInSeconds = 0;
        float TriggerChance = 0;
        FName AssociatedActor;
        Ar << EventId;
        Ar.SkipBytes(2);
        Ar << DelayInSeconds << TriggerChance << AssociatedActor;
        const int32 Count = Ar.ReadCount(1);
        Steps[Index].EventId = EventId;
        Steps[Index].DelayInSeconds = DelayInSeconds;
        Steps[Index].TriggerChance = TriggerChance;
        Steps[Index].AssociatedActor = AssociatedActor;
        for (int32 Step = 0; Step < Count && !Ar.IsError(); Step++)
            Inner.Add(SerializeTypedStep(Ar, Depth));
        break;
    }
    case 3: {
        Ar.SkipBytes(3);
        const int32 Additional = SerializeTypedStep(Ar, Depth);
        Steps[Index].AdditionalSequence = Additional;
        const int32 Count = Ar.ReadCount(1);
        for (int32 Step = 0; Step < Count && !Ar.IsError(); Step++)
            Inner.Add(SerializeTypedStep(Ar, Depth));
        break;
    }
    default: {
        Ar.SkipBytes(1);
        const int32 Count = Ar.ReadCount(1);
        for (int32 Step = 0; Step < Count && !Ar.IsError(); Step++) {
            SkipFixedArray(Ar, 12);
            Inner.Add(SerializeTypedStep(Ar, Depth));
        }
        break;
    }
    }
    Steps[Index].InnerSequence = std::move(Inner);
    return Index;
}

}
