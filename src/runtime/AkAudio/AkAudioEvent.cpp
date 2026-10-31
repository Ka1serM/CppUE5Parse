#include "ue5parse/runtime/AkAudio/AkAudioEvent.h"

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

// UE's AkAudioEvent.cpp (Wwise 2022.1): a cooked event serializes its FWwiseLocalizedEventCookedData after the
// properties. The struct is laid out from the mappings, then copied into the C++ values.

namespace ue5parse::runtime {

namespace {

template <class T>
T Member(const UStruct* Struct, const void* Value, const char* Name) {
    T Result{};
    GetPropertyValue_InContainer(Struct, Value, FName(Name), Result);
    return Result;
}

// The elements of a struct array member, each with its struct.
template <class Read>
void ForEachElement(const UStruct* Struct, const void* Value, const char* Name, Read ReadElement) {
    const FArrayProperty* Array = FindFProperty<FArrayProperty>(Struct, FName(Name));
    const FStructProperty* Inner = Array ? CastField<FStructProperty>(Array->Inner) : nullptr;
    if (Inner == nullptr) return;
    FScriptArrayHelper Helper(Array, Array->ContainerPtrToValuePtr<void>(Value));
    for (int32 i = 0; i < Helper.Num(); i++) ReadElement(Inner->Struct, Helper.GetRawPtr(i));
}

FWwiseMediaCookedData ReadMedia(const UStruct* Struct, const void* Value) {
    return {.MediaId = Member<int32>(Struct, Value, "MediaId"),
            .MediaPathName = Member<FName>(Struct, Value, "MediaPathName"),
            .PrefetchSize = Member<int32>(Struct, Value, "PrefetchSize"),
            .MemoryAlignment = Member<int32>(Struct, Value, "MemoryAlignment"),
            .bDeviceMemory = Member<bool>(Struct, Value, "bDeviceMemory"),
            .bStreaming = Member<bool>(Struct, Value, "bStreaming"),
            .DebugName = Member<FName>(Struct, Value, "DebugName")};
}

TArray<FWwiseMediaCookedData> ReadMediaArray(const UStruct* Struct, const void* Value) {
    TArray<FWwiseMediaCookedData> Media;
    ForEachElement(Struct, Value, "Media", [&](const UStruct* Inner, const void* Element) { Media.Add(ReadMedia(Inner, Element)); });
    return Media;
}

FWwiseEventCookedData ReadEvent(const UStruct* Struct, const void* Value) {
    FWwiseEventCookedData Event{.EventId = Member<int32>(Struct, Value, "EventId"),
                                .Media = ReadMediaArray(Struct, Value),
                                .SwitchContainerLeaves = {},
                                .DebugName = Member<FName>(Struct, Value, "DebugName")};
    ForEachElement(Struct, Value, "SwitchContainerLeaves", [&](const UStruct* Inner, const void* Element) {
        Event.SwitchContainerLeaves.Add({.Media = ReadMediaArray(Inner, Element)});
    });
    return Event;
}

FWwiseLanguageCookedData ReadLanguage(const UStruct* Struct, const void* Value) {
    return {.LanguageId = Member<int32>(Struct, Value, "LanguageId"),
            .LanguageName = Member<FName>(Struct, Value, "LanguageName"),
            .LanguageRequirement = static_cast<EWwiseLanguageRequirement>(Member<uint8>(Struct, Value, "LanguageRequirement"))};
}

FWwiseLocalizedEventCookedData ReadLocalized(const UStruct* Struct, const void* Value) {
    FWwiseLocalizedEventCookedData Localized{.EventLanguageMap = {},
                                             .DebugName = Member<FName>(Struct, Value, "DebugName"),
                                             .EventId = Member<int32>(Struct, Value, "EventId")};
    const FMapProperty* Map = FindFProperty<FMapProperty>(Struct, FName("EventLanguageMap"));
    const FStructProperty* Key = Map ? CastField<FStructProperty>(Map->KeyProp) : nullptr;
    const FStructProperty* Data = Map ? CastField<FStructProperty>(Map->ValueProp) : nullptr;
    if (Key == nullptr || Data == nullptr) return Localized;
    FScriptMapHelper Helper(Map, Map->ContainerPtrToValuePtr<void>(Value));
    for (int32 i = 0; i < Helper.GetMaxIndex(); i++) {
        if (!Helper.IsValidIndex(i)) continue;
        Localized.EventLanguageMap.Add({ReadLanguage(Key->Struct, Helper.GetKeyPtr(i)), ReadEvent(Data->Struct, Helper.GetValuePtr(i))});
    }
    return Localized;
}

}

void UAkAudioEvent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || HasAnyFlags(RF_ClassDefaultObject)) return;
    // The mapped property is transient: the tagged properties leave it empty, and it holds the data read here.
    const FStructProperty* Property = FindFProperty<FStructProperty>(GetClass(), FName("EventCookedData"));
    if (Property == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "UAkAudioEvent.EventCookedData");
        return;
    }
    void* Value = Property->ContainerPtrToValuePtr<void>(this);
    Property->Struct->SerializeItem(Ar, Value);
    if (Ar.IsError()) return;
    EventCookedData = ReadLocalized(Property->Struct, Value);
}

}
