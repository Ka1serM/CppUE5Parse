#include <cstring>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/CoreRedirects.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

constexpr FName NAME_StructProperty("StructProperty");
constexpr FName NAME_ByteProperty("ByteProperty");
constexpr FName NAME_ArrayProperty("ArrayProperty");

}

void FStructProperty::LinkInternal() {
    UE5PARSE_CHECK(Struct != nullptr && Struct->IsLinked());
    ElementSize = Struct->GetStructureSize();
    EPropertyFlags Flags = PropertyFlags & ~CPF_ComputedFlags;
    if (Struct->StructFlags & STRUCT_IsPlainOldData)
        Flags = Flags | CPF_IsPlainOldData;
    if (Struct->StructFlags & STRUCT_NoDestructor)
        Flags = Flags | CPF_NoDestructor;
    if (Struct->StructFlags & STRUCT_ZeroConstructor)
        Flags = Flags | CPF_ZeroConstructor;
    if (Struct->CppStructOps && Struct->CppStructOps->GetTypeHash)
        Flags = Flags | CPF_HasGetValueTypeHash;
    PropertyFlags = Flags;
}

int32 FStructProperty::GetMinAlignment() const {
    return Struct->GetMinAlignment();
}

bool FStructProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    return Struct->CompareScriptStruct(A, B, PortFlags);
}

uint32 FStructProperty::GetValueTypeHashInternal(const void* Src) const {
    return Struct->GetStructTypeHash(Src);
}

void FStructProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    Struct->CopyScriptStruct(Dest, Src, Count);
}

void FStructProperty::ClearValueInternal(void* Data) const {
    Struct->ClearScriptStruct(Data, 1);
}

void FStructProperty::DestroyValueInternal(void* Dest) const {
    Struct->DestroyStruct(Dest, ArrayDim);
}

void FStructProperty::InitializeValueInternal(void* Dest) const {
    std::memset(Dest, 0, static_cast<std::size_t>(ElementSize) * ArrayDim);
    Struct->InitializeStruct(Dest, ArrayDim);
}

bool FStructProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && static_cast<const FStructProperty*>(Other)->Struct == Struct;
}

void FStructProperty::SerializeItem(FArchive& Ar, void* Value) const {
    Struct->SerializeItem(Ar, Value);
}

bool FStructProperty::CanSerializeFromStructName(FName StructName) const {
    return StructName == Struct->GetFName() || FCoreRedirects::GetRedirectedStructName(StructName) == Struct->GetFName();
}

EConvertFromTypeResult FStructProperty::ConvertFromType(const FPropertyTag& Tag, FArchive& Ar, uint8* Data) const {
    const bool bCanSerialize = Tag.Type == NAME_StructProperty && CanSerializeFromStructName(Tag.TypeName.GetParameterName(0));
    const FCppStructOps* CppStructOps = Struct->GetCppStructOps();
    if (!bCanSerialize && CppStructOps != nullptr && CppStructOps->SerializeFromMismatchedTag != nullptr) {
        if (CppStructOps->SerializeFromMismatchedTag(Tag, Ar, ContainerPtrToValuePtr<void>(Data, Tag.ArrayIndex)))
            return EConvertFromTypeResult::Converted;
        return EConvertFromTypeResult::CannotConvert;
    }
    if (Tag.Type == NAME_StructProperty && !bCanSerialize)
        return EConvertFromTypeResult::CannotConvert;
    return EConvertFromTypeResult::UseSerializeItem;
}

void FArrayProperty::LinkInternal() {
    Inner->Link();
    ElementSize = sizeof(FScriptArray);
    PropertyFlags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_ZeroConstructor;
}

bool FArrayProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    FScriptArrayHelper ArrayA(this, A);
    if (B == nullptr)
        return ArrayA.Num() == 0;
    FScriptArrayHelper ArrayB(this, B);
    if (ArrayA.Num() != ArrayB.Num())
        return false;
    for (int32 i = 0; i < ArrayA.Num(); i++)
        if (!Inner->Identical(ArrayA.GetRawPtr(i), ArrayB.GetRawPtr(i), PortFlags))
            return false;
    return true;
}

void FArrayProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        const void* SrcArray = static_cast<const uint8*>(Src) + static_cast<std::size_t>(i) * ElementSize;
        void* DestArray = static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize;
        if (SrcArray == DestArray)
            continue;
        FScriptArrayHelper SrcHelper(this, SrcArray);
        FScriptArrayHelper DestHelper(this, DestArray);
        int32 Num = SrcHelper.Num();
        if (Inner->PropertyFlags & CPF_IsPlainOldData) {
            DestHelper.EmptyValues(Num);
            DestHelper.AddUninitializedValues(Num);
            if (Num)
                std::memcpy(DestHelper.GetRawPtr(), SrcHelper.GetRawPtr(), static_cast<std::size_t>(Num) * Inner->GetSize());
        } else {
            DestHelper.EmptyAndAddValues(Num);
            for (int32 k = 0; k < Num; k++)
                Inner->CopyCompleteValue(DestHelper.GetRawPtr(k), SrcHelper.GetRawPtr(k));
        }
    }
}

void FArrayProperty::ClearValueInternal(void* Data) const {
    FScriptArrayHelper(this, Data).EmptyValues();
}

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
    if (Count > Ar.GetRemaining() && Inner->ElementSize > 0) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FArrayProperty.Count");
        Helper.EmptyValues();
        return;
    }
    if (const FStructProperty* InnerStruct = CastField<FStructProperty>(Inner); InnerStruct != nullptr && HasInnerTag(Ar)) {
        FPropertyTag InnerTag;
        Ar << InnerTag;
        if (Ar.IsError())
            return;
        const FCppStructOps* InnerOps = InnerStruct->Struct->GetCppStructOps();
        if (!InnerStruct->CanSerializeFromStructName(InnerTag.TypeName.GetParameterName(0)) && InnerOps != nullptr
            && InnerOps->SerializeFromMismatchedTag != nullptr) {
            Helper.EmptyAndAddValues(Count);
            for (int32 i = 0; i < Count && !Ar.IsError(); i++)
                if (InnerStruct->ConvertFromType(InnerTag, Ar, Helper.GetRawPtr(i)) != EConvertFromTypeResult::Converted)
                    Ar.SetError(ErrorCode::InvalidData, "FArrayProperty.Inner");
            return;
        }
        if (!InnerStruct->CanSerializeFromStructName(InnerTag.TypeName.GetParameterName(0))) {
            ReportLoadProblem("load.tag_convert", GetName() + FString(" stores ") + FString(InnerTag.TypeName.GetParameterName(0).toString())
                                                      + FString(", not ") + InnerStruct->Struct->GetName());
            Helper.EmptyValues();
            Ar.SkipBytes(InnerTag.Size);
            return;
        }
    }
    Helper.EmptyAndAddValues(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++)
        Inner->SerializeItem(Ar, Helper.GetRawPtr(i));
}

bool FArrayProperty::HasInnerTag(const FArchive& Ar) {
    return !Ar.UseUnversionedPropertySerialization() && Ar.UEVer() >= EUnrealEngineObjectUE4Version::INNER_ARRAY_TAG_INFO
           && Ar.UEVer() < EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME;
}

EConvertFromTypeResult FArrayProperty::ConvertFromType(const FPropertyTag& Tag, FArchive&, uint8*) const {
    FName InnerType = Tag.TypeName.GetParameterName(0);
    if (Tag.Type == NAME_ArrayProperty && !InnerType.isNone() && InnerType != Inner->GetID())
        return EConvertFromTypeResult::CannotConvert;
    return EConvertFromTypeResult::UseSerializeItem;
}

void FArrayProperty::ResolveObjectReferences(void* Value) const {
    if (!Inner->IsA<FObjectProperty>())
        return;
    FScriptArrayHelper Helper(this, Value);
    for (int32 i = 0; i < Helper.Num(); i++)
        Inner->ResolveObjectReferences(Helper.GetRawPtr(i));
}

void FSetProperty::LinkInternal() {
    ElementProp->Link();
    SetLayout = GetScriptSetLayout(ElementProp->GetSize(), ElementProp->GetMinAlignment());
    ElementSize = sizeof(FScriptSet);
    PropertyFlags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_ZeroConstructor;
}

bool FSetProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    FScriptSetHelper SetA(this, A);
    if (B == nullptr)
        return SetA.Num() == 0;
    FScriptSetHelper SetB(this, B);
    if (SetA.Num() != SetB.Num())
        return false;
    for (int32 Index = 0; Index < SetA.GetMaxIndex(); Index++) {
        if (!SetA.IsValidIndex(Index))
            continue;
        if (SetB.FindElementIndex(SetA.GetElementPtr(Index)) == INDEX_NONE)
            return false;
    }
    (void)PortFlags;
    return true;
}

void FSetProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        const void* SrcSet = static_cast<const uint8*>(Src) + static_cast<std::size_t>(i) * ElementSize;
        void* DestSet = static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize;
        if (SrcSet == DestSet)
            continue;
        FScriptSetHelper SrcHelper(this, SrcSet);
        FScriptSetHelper DestHelper(this, DestSet);
        DestHelper.EmptyElements(SrcHelper.Num());
        for (int32 Index = 0; Index < SrcHelper.GetMaxIndex(); Index++) {
            if (!SrcHelper.IsValidIndex(Index))
                continue;
            int32 DestIndex = DestHelper.AddDefaultValue_Invalid_NeedsRehash();
            ElementProp->CopySingleValue(DestHelper.GetElementPtr(DestIndex), SrcHelper.GetElementPtr(Index));
        }
        DestHelper.Rehash();
    }
}

void FSetProperty::ClearValueInternal(void* Data) const {
    FScriptSetHelper(this, Data).EmptyElements();
}

void FSetProperty::DestroyValueInternal(void* Dest) const {
    for (int32 i = 0; i < ArrayDim; i++)
        FScriptSetHelper(this, static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize).EmptyElements();
}

bool FSetProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && ElementProp->SameType(static_cast<const FSetProperty*>(Other)->ElementProp);
}

void FSetProperty::SerializeItem(FArchive& Ar, void* Value) const {
    FScriptSetHelper Helper(this, Value);
    FDefaultConstructedPropertyElement Element(ElementProp);
    const int32 NumElementsToRemove = Ar.ReadCount(0);
    for (int32 i = 0; i < NumElementsToRemove; i++) {
        ElementProp->SerializeItem(Ar, Element.GetObjAddress());
        if (Ar.IsError())
            return;
        Helper.RemoveElement(Element.GetObjAddress());
    }
    const int32 Num = Ar.ReadCount(0);
    if (Num > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FSetProperty.Count");
        return;
    }
    for (int32 i = 0; i < Num; i++) {
        ElementProp->SerializeItem(Ar, Element.GetObjAddress());
        if (Ar.IsError())
            return;
        Helper.AddElement(Element.GetObjAddress());
    }
}

void FSetProperty::ResolveObjectReferences(void* Value) const {
    if (!ElementProp->IsA<FObjectProperty>())
        return;
    FScriptSetHelper Helper(this, Value);
    for (int32 i = 0; i < Helper.GetMaxIndex(); i++)
        if (Helper.IsValidIndex(i))
            ElementProp->ResolveObjectReferences(Helper.GetElementPtr(i));
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
    if (B == nullptr)
        return MapA.Num() == 0;
    FScriptMapHelper MapB(this, B);
    if (MapA.Num() != MapB.Num())
        return false;
    for (int32 Index = 0; Index < MapA.GetMaxIndex(); Index++) {
        if (!MapA.IsValidIndex(Index))
            continue;
        uint8* ValueB = MapB.FindValueFromHash(MapA.GetKeyPtr(Index));
        if (ValueB == nullptr || !ValueProp->Identical(MapA.GetValuePtr(Index), ValueB, PortFlags))
            return false;
    }
    return true;
}

void FMapProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        const void* SrcMap = static_cast<const uint8*>(Src) + static_cast<std::size_t>(i) * ElementSize;
        void* DestMap = static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize;
        if (SrcMap == DestMap)
            continue;
        FScriptMapHelper SrcHelper(this, SrcMap);
        FScriptMapHelper DestHelper(this, DestMap);
        DestHelper.EmptyValues(SrcHelper.Num());
        for (int32 Index = 0; Index < SrcHelper.GetMaxIndex(); Index++) {
            if (!SrcHelper.IsValidIndex(Index))
                continue;
            int32 DestIndex = DestHelper.AddDefaultValue_Invalid_NeedsRehash();
            KeyProp->CopySingleValue(DestHelper.GetKeyPtr(DestIndex), SrcHelper.GetKeyPtr(Index));
            ValueProp->CopySingleValue(DestHelper.GetValuePtr(DestIndex), SrcHelper.GetValuePtr(Index));
        }
        DestHelper.Rehash();
    }
}

void FMapProperty::ClearValueInternal(void* Data) const {
    FScriptMapHelper(this, Data).EmptyValues();
}

void FMapProperty::DestroyValueInternal(void* Dest) const {
    for (int32 i = 0; i < ArrayDim; i++)
        FScriptMapHelper(this, static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize).EmptyValues();
}

bool FMapProperty::SameType(const FProperty* Other) const {
    if (!FProperty::SameType(Other))
        return false;
    const FMapProperty* OtherMap = static_cast<const FMapProperty*>(Other);
    return KeyProp->SameType(OtherMap->KeyProp) && ValueProp->SameType(OtherMap->ValueProp);
}

void FMapProperty::SerializeItem(FArchive& Ar, void* Value) const {
    FScriptMapHelper Helper(this, Value);
    FDefaultConstructedPropertyElement Key(KeyProp);
    const int32 NumKeysToRemove = Ar.ReadCount(0);
    for (int32 i = 0; i < NumKeysToRemove; i++) {
        KeyProp->SerializeItem(Ar, Key.GetObjAddress());
        if (Ar.IsError())
            return;
        Helper.RemovePair(Key.GetObjAddress());
    }
    const int32 Num = Ar.ReadCount(0);
    if (Num > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FMapProperty.Count");
        return;
    }
    for (int32 i = 0; i < Num; i++) {
        KeyProp->SerializeItem(Ar, Key.GetObjAddress());
        if (Ar.IsError())
            return;
        ValueProp->SerializeItem(Ar, Helper.FindOrAdd(Key.GetObjAddress()));
    }
}

namespace {

bool SerializeIntegerOfType(FArchive& Ar, FName Type, int64& Value) {
    if (Type == FName("Int8Property")) {
        int8 Read = 0;
        Ar << Read;
        Value = Read;
        return true;
    }
    if (Type == FName("Int16Property")) {
        int16 Read = 0;
        Ar << Read;
        Value = Read;
        return true;
    }
    if (Type == FName("IntProperty")) {
        int32 Read = 0;
        Ar << Read;
        Value = Read;
        return true;
    }
    if (Type == FName("UInt16Property")) {
        uint16 Read = 0;
        Ar << Read;
        Value = Read;
        return true;
    }
    if (Type == FName("UInt32Property")) {
        uint32 Read = 0;
        Ar << Read;
        Value = Read;
        return true;
    }
    if (Type == FName("Int64Property") || Type == FName("UInt64Property")) {
        Ar << Value;
        return true;
    }
    return false;
}

}

EConvertFromTypeResult FMapProperty::ConvertFromType(const FPropertyTag& Tag, FArchive& Ar, uint8* Data) const {
    const FName KeyType = Tag.TypeName.GetParameterName(0);
    if (Tag.Type != FName("MapProperty") || KeyType.isNone() || KeyType == KeyProp->GetID())
        return EConvertFromTypeResult::UseSerializeItem;
    const FNumericProperty* NumericKey = CastField<FNumericProperty>(KeyProp);
    if (NumericKey != nullptr && !NumericKey->IsInteger())
        return EConvertFromTypeResult::CannotConvert;
    if ((NumericKey == nullptr && !KeyProp->IsA<FStructProperty>()) || Tag.TypeName.GetParameterName(1) != ValueProp->GetID())
        return EConvertFromTypeResult::CannotConvert;
    FScriptMapHelper Helper(this, ContainerPtrToValuePtr<void>(Data, Tag.ArrayIndex));
    FDefaultConstructedPropertyElement Key(KeyProp);
    FPropertyTag KeyTag;
    KeyTag.Name = Tag.Name;
    KeyTag.Type = KeyType;
    const auto ReadKey = [&] {
        if (NumericKey == nullptr)
            return KeyProp->ConvertFromType(KeyTag, Ar, static_cast<uint8*>(Key.GetObjAddress())) == EConvertFromTypeResult::Converted;
        int64 KeyValue = 0;
        if (!SerializeIntegerOfType(Ar, KeyType, KeyValue))
            return false;
        NumericKey->SetIntPropertyValue(Key.GetObjAddress(), KeyValue);
        return true;
    };
    const int32 NumKeysToRemove = Ar.ReadCount(0);
    for (int32 i = 0; i < NumKeysToRemove; i++) {
        if (!ReadKey())
            return EConvertFromTypeResult::CannotConvert;
        Helper.RemovePair(Key.GetObjAddress());
    }
    const int32 Num = Ar.ReadCount(0);
    if (Num > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "FMapProperty.Count");
        return EConvertFromTypeResult::Converted;
    }
    for (int32 i = 0; i < Num; i++) {
        if (!ReadKey())
            return EConvertFromTypeResult::CannotConvert;
        if (Ar.IsError())
            return EConvertFromTypeResult::Converted;
        ValueProp->SerializeItem(Ar, Helper.FindOrAdd(Key.GetObjAddress()));
    }
    return EConvertFromTypeResult::Converted;
}

void FMapProperty::ResolveObjectReferences(void* Value) const {
    const bool bKeys = KeyProp->IsA<FObjectProperty>();
    const bool bValues = ValueProp->IsA<FObjectProperty>();
    if (!bKeys && !bValues)
        return;
    FScriptMapHelper Helper(this, Value);
    for (int32 i = 0; i < Helper.GetMaxIndex(); i++) {
        if (!Helper.IsValidIndex(i))
            continue;
        if (bKeys)
            KeyProp->ResolveObjectReferences(Helper.GetKeyPtr(i));
        if (bValues)
            ValueProp->ResolveObjectReferences(Helper.GetValuePtr(i));
    }
}

void FEnumProperty::LinkInternal() {
    UnderlyingProp->Link();
    ElementSize = UnderlyingProp->ElementSize;
    PropertyFlags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_IsPlainOldData | CPF_NoDestructor | CPF_ZeroConstructor | CPF_HasGetValueTypeHash;
}

bool FEnumProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && static_cast<const FEnumProperty*>(Other)->Enum == Enum;
}

EConvertFromTypeResult FEnumProperty::ConvertFromType(const FPropertyTag& Tag, FArchive& Ar, uint8* Data) const {
    if (Tag.Type != NAME_ByteProperty)
        return EConvertFromTypeResult::UseSerializeItem;
    void* Value = ContainerPtrToValuePtr<void>(Data, Tag.ArrayIndex);
    if (Tag.TypeName.GetParameterCount() > 0) {
        SerializeItem(Ar, Value);
        return EConvertFromTypeResult::Serialized;
    }
    uint8 RawValue = 0;
    Ar << RawValue;
    UnderlyingProp->SetIntPropertyValue(Value, static_cast<uint64>(RawValue));
    return EConvertFromTypeResult::Converted;
}

void FEnumProperty::SerializeItem(FArchive& Ar, void* Value) const {
    FName EntryName;
    Ar << EntryName;
    int64 EnumValue = Enum ? Enum->GetValueByName(EntryName) : INDEX_NONE;
    if (EnumValue == INDEX_NONE)
        EnumValue = 0;
    UnderlyingProp->SetIntPropertyValue(Value, EnumValue);
}

void FOptionalProperty::LinkInternal() {
    ValueProperty->Link();
    const int32 Alignment = ValueProperty->GetMinAlignment();
    ElementSize = (GetIsSetOffset() + 1 + Alignment - 1) / Alignment * Alignment;
    EPropertyFlags Flags = (PropertyFlags & ~CPF_ComputedFlags) | CPF_ZeroConstructor | CPF_HasGetValueTypeHash;
    if (ValueProperty->PropertyFlags & CPF_IsPlainOldData)
        Flags = Flags | CPF_IsPlainOldData;
    if (ValueProperty->PropertyFlags & CPF_NoDestructor)
        Flags = Flags | CPF_NoDestructor;
    PropertyFlags = Flags;
}

void* FOptionalProperty::MarkSetAndGetInitializedValuePointerToReplace(void* Data) const {
    uint8& Set = static_cast<uint8*>(Data)[GetIsSetOffset()];
    if (Set == 0) {
        ValueProperty->InitializeValue(Data);
        Set = 1;
    }
    return Data;
}

void FOptionalProperty::MarkUnset(void* Data) const {
    uint8& Set = static_cast<uint8*>(Data)[GetIsSetOffset()];
    if (Set == 0)
        return;
    ValueProperty->DestroyValue(Data);
    Set = 0;
}

bool FOptionalProperty::Identical(const void* A, const void* B, uint32 PortFlags) const {
    if (B == nullptr)
        return !IsSet(A);
    if (IsSet(A) != IsSet(B))
        return false;
    return !IsSet(A) || ValueProperty->Identical(A, B, PortFlags);
}

uint32 FOptionalProperty::GetValueTypeHashInternal(const void* Src) const {
    return IsSet(Src) ? ValueProperty->GetValueTypeHash(Src) : 0;
}

void FOptionalProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        const void* SrcValue = static_cast<const uint8*>(Src) + static_cast<std::size_t>(i) * ElementSize;
        void* DestValue = static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize;
        if (IsSet(SrcValue))
            ValueProperty->CopySingleValue(MarkSetAndGetInitializedValuePointerToReplace(DestValue), SrcValue);
        else
            MarkUnset(DestValue);
    }
}

void FOptionalProperty::ClearValueInternal(void* Data) const {
    MarkUnset(Data);
}

void FOptionalProperty::DestroyValueInternal(void* Dest) const {
    for (int32 i = 0; i < ArrayDim; i++)
        MarkUnset(static_cast<uint8*>(Dest) + static_cast<std::size_t>(i) * ElementSize);
}

void FOptionalProperty::InitializeValueInternal(void* Dest) const {
    std::memset(Dest, 0, static_cast<std::size_t>(ElementSize) * ArrayDim);
}

bool FOptionalProperty::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && ValueProperty->SameType(static_cast<const FOptionalProperty*>(Other)->ValueProperty);
}

void FOptionalProperty::SerializeItem(FArchive& Ar, void* Value) const {
    bool bIsSet = false;
    Ar << bIsSet;
    if (Ar.IsError())
        return;
    if (!bIsSet) {
        MarkUnset(Value);
        return;
    }
    void* ValueData = MarkSetAndGetInitializedValuePointerToReplace(Value);
    if (Ar.UseUnversionedPropertySerialization() || Ar.UEVer() >= EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME) {
        ValueProperty->SerializeItem(Ar, ValueData);
        return;
    }
    FPropertyTag ValueTag;
    Ar << ValueTag;
    if (Ar.IsError())
        return;
    ValueTag.ArrayIndex = 0;
    const int64 StartOfValue = Ar.Tell();
    switch (ValueProperty->ConvertFromType(ValueTag, Ar, static_cast<uint8*>(ValueData))) {
    case EConvertFromTypeResult::Serialized:
    case EConvertFromTypeResult::Converted:
        return;
    case EConvertFromTypeResult::CannotConvert:
        break;
    case EConvertFromTypeResult::UseSerializeItem:
        if (ValueTag.Type != ValueProperty->GetID())
            break;
        ValueTag.SerializeTaggedProperty(Ar, ValueProperty, static_cast<uint8*>(ValueData));
        return;
    }
    ReportLoadProblem("load.tag_convert",
                      GetName() + FString(" stores ") + FString(ValueTag.Type.toString()) + FString(", not ") + FString(ValueProperty->GetID().toString()));
    MarkUnset(Value);
    Ar.Seek(StartOfValue + ValueTag.Size);
}

EConvertFromTypeResult FOptionalProperty::ConvertFromType(const FPropertyTag& Tag, FArchive&, uint8*) const {
    const FName ValueType = Tag.TypeName.GetParameterName(0);
    if (Tag.Type == FName("OptionalProperty") && !ValueType.isNone() && ValueType != ValueProperty->GetID())
        return EConvertFromTypeResult::CannotConvert;
    return EConvertFromTypeResult::UseSerializeItem;
}

void FOptionalProperty::ResolveObjectReferences(void* Value) const {
    if (IsSet(Value))
        ValueProperty->ResolveObjectReferences(Value);
}

}
