#include <cstring>

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

// FStructProperty, FArrayProperty, FSetProperty, FMapProperty and FEnumProperty (UE's PropertyStruct.cpp,
// PropertyArray.cpp, PropertySet.cpp, PropertyMap.cpp, EnumProperty.cpp).

namespace ue5parse::runtime {

void FStructProperty::LinkInternal() {
    UE5PARSE_CHECK(Struct != nullptr && Struct->IsLinked());
    ElementSize = Struct->GetStructureSize();
    EPropertyFlags Flags = PropertyFlags & ~CPF_ComputedFlags;
    if (Struct->StructFlags & STRUCT_IsPlainOldData) Flags = Flags | CPF_IsPlainOldData;
    if (Struct->StructFlags & STRUCT_NoDestructor) Flags = Flags | CPF_NoDestructor;
    if (Struct->StructFlags & STRUCT_ZeroConstructor) Flags = Flags | CPF_ZeroConstructor;
    if (Struct->CppStructOps && Struct->CppStructOps->GetTypeHash) Flags = Flags | CPF_HasGetValueTypeHash;
    PropertyFlags = Flags;
}

int32 FStructProperty::GetMinAlignment() const { return Struct->GetMinAlignment(); }

bool FStructProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    return Struct->CompareScriptStruct(A, B, PortFlags);
}

uint32 FStructProperty::GetValueTypeHashInternal(const void* Src) const { return Struct->GetStructTypeHash(Src); }

void FStructProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const { Struct->CopyScriptStruct(Dest, Src, Count); }

void FStructProperty::ClearValueInternal(void* Data) const { Struct->ClearScriptStruct(Data, 1); }

void FStructProperty::DestroyValueInternal(void* Dest) const { Struct->DestroyStruct(Dest, ArrayDim); }

void FStructProperty::InitializeValueInternal(void* Dest) const {
    std::memset(Dest, 0, static_cast<std::size_t>(ElementSize) * ArrayDim);
    Struct->InitializeStruct(Dest, ArrayDim);
}

bool FStructProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && static_cast<const FStructProperty*>(Other)->Struct == Struct;
}

void FStructProperty::SerializeItem(FArchive& Ar, void* Value) const { Struct->SerializeItem(Ar, Value); }

void FArrayProperty::LinkInternal() {
    Inner->Link();
    ElementSize = sizeof(FScriptArray);
    PropertyFlags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_ZeroConstructor;
}

bool FArrayProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    FScriptArrayHelper ArrayA(this, A);
    if (B == nullptr) return ArrayA.Num() == 0;
    FScriptArrayHelper ArrayB(this, B);
    if (ArrayA.Num() != ArrayB.Num()) return false;
    for (int32 i = 0; i < ArrayA.Num(); i++)
        if (!Inner->Identical(ArrayA.GetRawPtr(i), ArrayB.GetRawPtr(i), PortFlags)) return false;
    return true;
}

void FArrayProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        const void* SrcArray = static_cast<const uint8*>(Src) + static_cast<std::size_t>(i) * ElementSize;
        void* DestArray = static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize;
        if (SrcArray == DestArray) continue;
        FScriptArrayHelper SrcHelper(this, SrcArray);
        FScriptArrayHelper DestHelper(this, DestArray);
        int32 Num = SrcHelper.Num();
        if (Inner->PropertyFlags & CPF_IsPlainOldData) {
            DestHelper.EmptyValues(Num);
            DestHelper.AddUninitializedValues(Num);
            if (Num) std::memcpy(DestHelper.GetRawPtr(), SrcHelper.GetRawPtr(), static_cast<std::size_t>(Num) * Inner->GetSize());
        } else {
            DestHelper.EmptyAndAddValues(Num);
            for (int32 k = 0; k < Num; k++) Inner->CopyCompleteValue(DestHelper.GetRawPtr(k), SrcHelper.GetRawPtr(k));
        }
    }
}

void FArrayProperty::ClearValueInternal(void* Data) const { FScriptArrayHelper(this, Data).EmptyValues(); }

void FArrayProperty::DestroyValueInternal(void* Dest) const {
    for (int32 i = 0; i < ArrayDim; i++)
        FScriptArrayHelper(this, static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize).EmptyValues();
}

bool FArrayProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && Inner->SameType(static_cast<const FArrayProperty*>(Other)->Inner);
}

void FArrayProperty::SerializeItem(FArchive& Ar, void* Value) const {
    FScriptArrayHelper Helper(this, Value);
    int32 Count = Ar.ReadCount(0);
    // Each element takes at least one byte, except zero-size structs.
    if (Count > Ar.GetRemaining() && Inner->ElementSize > 0) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FArrayProperty.Count");
        Helper.EmptyValues();
        return;
    }
    Helper.EmptyAndAddValues(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) Inner->SerializeItem(Ar, Helper.GetRawPtr(i));
}

void FArrayProperty::ResolveObjectReferences(void* Value) const {
    if (!Inner->IsA<FObjectProperty>()) return;
    FScriptArrayHelper Helper(this, Value);
    for (int32 i = 0; i < Helper.Num(); i++) Inner->ResolveObjectReferences(Helper.GetRawPtr(i));
}

void FSetProperty::LinkInternal() {
    ElementProp->Link();
    SetLayout = GetScriptSetLayout(ElementProp->GetSize(), ElementProp->GetMinAlignment());
    ElementSize = sizeof(FScriptSet);
    PropertyFlags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_ZeroConstructor;
}

bool FSetProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    FScriptSetHelper SetA(this, A);
    if (B == nullptr) return SetA.Num() == 0;
    FScriptSetHelper SetB(this, B);
    if (SetA.Num() != SetB.Num()) return false;
    for (int32 Index = 0; Index < SetA.GetMaxIndex(); Index++) {
        if (!SetA.IsValidIndex(Index)) continue;
        if (SetB.FindElementIndex(SetA.GetElementPtr(Index)) == INDEX_NONE) return false;
    }
    (void)PortFlags;
    return true;
}

void FSetProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        const void* SrcSet = static_cast<const uint8*>(Src) + static_cast<std::size_t>(i) * ElementSize;
        void* DestSet = static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize;
        if (SrcSet == DestSet) continue;
        FScriptSetHelper SrcHelper(this, SrcSet);
        FScriptSetHelper DestHelper(this, DestSet);
        DestHelper.EmptyElements(SrcHelper.Num());
        for (int32 Index = 0; Index < SrcHelper.GetMaxIndex(); Index++) {
            if (!SrcHelper.IsValidIndex(Index)) continue;
            int32 DestIndex = DestHelper.AddDefaultValue_Invalid_NeedsRehash();
            ElementProp->CopySingleValue(DestHelper.GetElementPtr(DestIndex), SrcHelper.GetElementPtr(Index));
        }
        DestHelper.Rehash();
    }
}

void FSetProperty::ClearValueInternal(void* Data) const { FScriptSetHelper(this, Data).EmptyElements(); }

void FSetProperty::DestroyValueInternal(void* Dest) const {
    for (int32 i = 0; i < ArrayDim; i++)
        FScriptSetHelper(this, static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize).EmptyElements();
}

bool FSetProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && ElementProp->SameType(static_cast<const FSetProperty*>(Other)->ElementProp);
}

void FSetProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // The value holds the defaults, as an object starts from its archetype: the elements to remove from them, then the
    // elements to add.
    FScriptSetHelper Helper(this, Value);
    FDefaultConstructedPropertyElement Element(ElementProp);
    const int32 NumElementsToRemove = Ar.ReadCount(0);
    for (int32 i = 0; i < NumElementsToRemove; i++) {
        ElementProp->SerializeItem(Ar, Element.GetObjAddress());
        if (Ar.IsError()) return;
        Helper.RemoveElement(Element.GetObjAddress());
    }
    const int32 Num = Ar.ReadCount(0);
    if (Num > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FSetProperty.Count");
        return;
    }
    for (int32 i = 0; i < Num; i++) {
        ElementProp->SerializeItem(Ar, Element.GetObjAddress());
        if (Ar.IsError()) return;
        Helper.AddElement(Element.GetObjAddress());
    }
}

void FSetProperty::ResolveObjectReferences(void* Value) const {
    if (!ElementProp->IsA<FObjectProperty>()) return;
    FScriptSetHelper Helper(this, Value);
    for (int32 i = 0; i < Helper.GetMaxIndex(); i++)
        if (Helper.IsValidIndex(i)) ElementProp->ResolveObjectReferences(Helper.GetElementPtr(i));
}

void FMapProperty::LinkInternal() {
    KeyProp->Link();
    ValueProp->Link();
    MapLayout = GetScriptMapLayout(KeyProp->GetSize(), KeyProp->GetMinAlignment(), ValueProp->GetSize(), ValueProp->GetMinAlignment());
    ElementSize = sizeof(FScriptMap);
    PropertyFlags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_ZeroConstructor;
}

bool FMapProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    FScriptMapHelper MapA(this, A);
    if (B == nullptr) return MapA.Num() == 0;
    FScriptMapHelper MapB(this, B);
    if (MapA.Num() != MapB.Num()) return false;
    for (int32 Index = 0; Index < MapA.GetMaxIndex(); Index++) {
        if (!MapA.IsValidIndex(Index)) continue;
        uint8* ValueB = MapB.FindValueFromHash(MapA.GetKeyPtr(Index));
        if (ValueB == nullptr || !ValueProp->Identical(MapA.GetValuePtr(Index), ValueB, PortFlags)) return false;
    }
    return true;
}

void FMapProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        const void* SrcMap = static_cast<const uint8*>(Src) + static_cast<std::size_t>(i) * ElementSize;
        void* DestMap = static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize;
        if (SrcMap == DestMap) continue;
        FScriptMapHelper SrcHelper(this, SrcMap);
        FScriptMapHelper DestHelper(this, DestMap);
        DestHelper.EmptyValues(SrcHelper.Num());
        for (int32 Index = 0; Index < SrcHelper.GetMaxIndex(); Index++) {
            if (!SrcHelper.IsValidIndex(Index)) continue;
            int32 DestIndex = DestHelper.AddDefaultValue_Invalid_NeedsRehash();
            KeyProp->CopySingleValue(DestHelper.GetKeyPtr(DestIndex), SrcHelper.GetKeyPtr(Index));
            ValueProp->CopySingleValue(DestHelper.GetValuePtr(DestIndex), SrcHelper.GetValuePtr(Index));
        }
        DestHelper.Rehash();
    }
}

void FMapProperty::ClearValueInternal(void* Data) const { FScriptMapHelper(this, Data).EmptyValues(); }

void FMapProperty::DestroyValueInternal(void* Dest) const {
    for (int32 i = 0; i < ArrayDim; i++)
        FScriptMapHelper(this, static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize).EmptyValues();
}

bool FMapProperty::SameType(const FProperty* Other) const {
    if (!FProperty::SameType(Other)) return false;
    const FMapProperty* OtherMap = static_cast<const FMapProperty*>(Other);
    return KeyProp->SameType(OtherMap->KeyProp) && ValueProp->SameType(OtherMap->ValueProp);
}

void FMapProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // The value holds the defaults, as an object starts from its archetype: the keys to remove from them, then the pairs
    // to add or change.
    FScriptMapHelper Helper(this, Value);
    FDefaultConstructedPropertyElement Key(KeyProp);
    const int32 NumKeysToRemove = Ar.ReadCount(0);
    for (int32 i = 0; i < NumKeysToRemove; i++) {
        KeyProp->SerializeItem(Ar, Key.GetObjAddress());
        if (Ar.IsError()) return;
        Helper.RemovePair(Key.GetObjAddress());
    }
    const int32 Num = Ar.ReadCount(0);
    if (Num > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FMapProperty.Count");
        return;
    }
    for (int32 i = 0; i < Num; i++) {
        KeyProp->SerializeItem(Ar, Key.GetObjAddress());
        if (Ar.IsError()) return;
        ValueProp->SerializeItem(Ar, Helper.FindOrAdd(Key.GetObjAddress()));
    }
}

void FMapProperty::ResolveObjectReferences(void* Value) const {
    const bool bKeys = KeyProp->IsA<FObjectProperty>();
    const bool bValues = ValueProp->IsA<FObjectProperty>();
    if (!bKeys && !bValues) return;
    FScriptMapHelper Helper(this, Value);
    for (int32 i = 0; i < Helper.GetMaxIndex(); i++) {
        if (!Helper.IsValidIndex(i)) continue;
        if (bKeys) KeyProp->ResolveObjectReferences(Helper.GetKeyPtr(i));
        if (bValues) ValueProp->ResolveObjectReferences(Helper.GetValuePtr(i));
    }
}

void FEnumProperty::LinkInternal() {
    UnderlyingProp->Link();
    ElementSize = UnderlyingProp->ElementSize;
    PropertyFlags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_IsPlainOldData | CPF_NoDestructor | CPF_ZeroConstructor |
                    CPF_HasGetValueTypeHash;
}

bool FEnumProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && static_cast<const FEnumProperty*>(Other)->Enum == Enum;
}

void FEnumProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // Inside a container or struct the entry's name ("EMyEnum::Value"); the top level of unversioned properties writes
    // the underlying number instead (UnversionedPropertySerialization.cpp).
    FName EntryName;
    Ar << EntryName;
    int64 EnumValue = Enum ? Enum->GetValueByName(EntryName) : INDEX_NONE;
    if (EnumValue == INDEX_NONE) EnumValue = 0;
    UnderlyingProp->SetIntPropertyValue(Value, EnumValue);
}

}
