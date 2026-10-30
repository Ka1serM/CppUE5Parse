#include <cstring>

#include "CoreUObjectPrivate.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

FFieldClass* FField::StaticClass() {
    static FFieldClass Class("FField", 0, 0, nullptr);
    return &Class;
}

IMPLEMENT_FIELD(FProperty)
IMPLEMENT_FIELD(FNumericProperty)
IMPLEMENT_FIELD(FInt8Property)
IMPLEMENT_FIELD(FInt16Property)
IMPLEMENT_FIELD(FIntProperty)
IMPLEMENT_FIELD(FInt64Property)
IMPLEMENT_FIELD(FUInt16Property)
IMPLEMENT_FIELD(FUInt32Property)
IMPLEMENT_FIELD(FUInt64Property)
IMPLEMENT_FIELD(FFloatProperty)
IMPLEMENT_FIELD(FDoubleProperty)
IMPLEMENT_FIELD(FByteProperty)
IMPLEMENT_FIELD(FBoolProperty)
IMPLEMENT_FIELD(FObjectPropertyBase)
IMPLEMENT_FIELD(FObjectProperty)
IMPLEMENT_FIELD(FClassProperty)
IMPLEMENT_FIELD(FWeakObjectProperty)
IMPLEMENT_FIELD(FLazyObjectProperty)
IMPLEMENT_FIELD(FSoftObjectProperty)
IMPLEMENT_FIELD(FSoftClassProperty)
IMPLEMENT_FIELD(FInterfaceProperty)
IMPLEMENT_FIELD(FNameProperty)
IMPLEMENT_FIELD(FStrProperty)
IMPLEMENT_FIELD(FTextProperty)
IMPLEMENT_FIELD(FStructProperty)
IMPLEMENT_FIELD(FArrayProperty)
IMPLEMENT_FIELD(FSetProperty)
IMPLEMENT_FIELD(FMapProperty)
IMPLEMENT_FIELD(FEnumProperty)
IMPLEMENT_FIELD(FDelegateProperty)
IMPLEMENT_FIELD(FMulticastDelegateProperty)
IMPLEMENT_FIELD(FMulticastInlineDelegateProperty)
IMPLEMENT_FIELD(FFieldPathProperty)

UObject* FField::GetOwnerUObject() const {
    FFieldVariant Current = Owner;
    while (!Current.IsUObject()) {
        if (Current.ToField() == nullptr) return nullptr;
        Current = Current.ToField()->Owner;
    }
    return Current.ToUObject();
}

UStruct* FField::GetOwnerStruct() const { return Cast<UStruct>(GetOwnerUObject()); }

UClass* FField::GetOwnerClass() const {
    // A function's property belongs to the function's class (UE's UField::GetOwnerClass).
    for (UObject* Object = GetOwnerUObject(); Object != nullptr; Object = Object->GetOuter())
        if (UClass* Class = Cast<UClass>(Object)) return Class;
    return nullptr;
}

FString FField::GetPathName() const {
    UObject* OwnerObject = GetOwnerUObject();
    FString Result = OwnerObject ? OwnerObject->GetPathName() : FString();
    Result += u":";
    Result += GetName();
    return Result;
}

int32 FProperty::SetupOffset(int32 OwnerPropertiesSize) {
    int32 Alignment = GetMinAlignment();
    Offset_Internal = (OwnerPropertiesSize + Alignment - 1) / Alignment * Alignment;
    return Offset_Internal + GetSize();
}

uint32 FProperty::GetValueTypeHashInternal(const void*) const { return 0; }

void FProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    std::memcpy(Dest, Src, static_cast<std::size_t>(ElementSize) * Count);
}

void FProperty::ClearValueInternal(void* Data) const { std::memset(Data, 0, ElementSize); }

void FProperty::DestroyValueInternal(void*) const {}

void FProperty::InitializeValueInternal(void* Dest) const { std::memset(Dest, 0, static_cast<std::size_t>(ElementSize) * ArrayDim); }

bool FProperty::SameType(const FProperty* Other) const { return Other && GetClass() == Other->GetClass(); }

template <class TCppType>
uint32 TProperty_Numeric<TCppType>::GetValueTypeHashInternal(const void* Src) const {
    TCppType Value = *static_cast<const TCppType*>(Src);
    if constexpr (std::is_floating_point_v<TCppType>) {
        // UE hashes the bits of a float or double (as GetTypeHash(float)).
        if constexpr (sizeof(TCppType) == 4) {
            uint32 Bits;
            std::memcpy(&Bits, &Value, 4);
            return Bits;
        } else {
            uint64 Bits;
            std::memcpy(&Bits, &Value, 8);
            return static_cast<uint32>(Bits) + (static_cast<uint32>(Bits >> 32) * 23);
        }
    } else if constexpr (sizeof(TCppType) == 8) {
        return static_cast<uint32>(Value) + (static_cast<uint32>(static_cast<uint64>(Value) >> 32) * 23);
    } else {
        return static_cast<uint32>(Value);
    }
}

template <class TCppType>
void TProperty_Numeric<TCppType>::SerializeItem(FArchive& Ar, void* Value) const {
    Ar.Serialize(Value, sizeof(TCppType));
}

template class TProperty_Numeric<int8>;
template class TProperty_Numeric<int16>;
template class TProperty_Numeric<int32>;
template class TProperty_Numeric<int64>;
template class TProperty_Numeric<uint8>;
template class TProperty_Numeric<uint16>;
template class TProperty_Numeric<uint32>;
template class TProperty_Numeric<uint64>;
template class TProperty_Numeric<float>;
template class TProperty_Numeric<double>;

void FByteProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // A TEnumAsByte inside a container or struct is written as the entry's name (the top level of unversioned
    // properties writes the number instead: UnversionedPropertySerialization.cpp).
    if (Enum == nullptr) {
        Ar.Serialize(Value, 1);
        return;
    }
    FName EntryName;
    Ar << EntryName;
    int64 EnumValue = Enum->GetValueByName(EntryName);
    *static_cast<uint8*>(Value) = EnumValue == INDEX_NONE ? 0 : static_cast<uint8>(EnumValue);
}

void FBoolProperty::SetBoolSize(uint32 InSize, bool bIsNativeBool) {
    FieldSize = static_cast<uint8>(InSize);
    ElementSize = static_cast<int32>(InSize);
    ByteOffset = 0;
    if (bIsNativeBool) {
        ByteMask = 1;
        FieldMask = 255;
    } else {
        ByteMask = 1;
        FieldMask = 1;
    }
}

void FBoolProperty::LinkInternal() {
    ElementSize = FieldSize;
    EPropertyFlags Flags = PropertyFlags & ~CPF_ComputedFlags;
    if (IsNativeBool()) {
        Flags = Flags | CPF_IsPlainOldData | CPF_NoDestructor | CPF_ZeroConstructor;
    } else {
        Flags = Flags | CPF_NoDestructor;
    }
    PropertyFlags = Flags | CPF_HasGetValueTypeHash;
}

bool FBoolProperty::Identical(const void* A, const void* B, uint32) const {
    bool ValueA = GetPropertyValue(A);
    bool ValueB = B ? GetPropertyValue(B) : false;
    return ValueA == ValueB;
}

void FBoolProperty::CopyValuesInternal(void* Dest, const void* Src, int32 Count) const {
    for (int32 i = 0; i < Count; i++) {
        SetPropertyValue(static_cast<uint8*>(Dest) + i * ElementSize,
                         GetPropertyValue(static_cast<const uint8*>(Src) + i * ElementSize));
    }
}

void FBoolProperty::InitializeValueInternal(void* Dest) const {
    for (int32 i = 0; i < ArrayDim; i++) SetPropertyValue(static_cast<uint8*>(Dest) + i * ElementSize, false);
}

void FBoolProperty::CopySingleValueToScriptVM(void* Dest, const void* Src) const {
    *static_cast<bool*>(Dest) = GetPropertyValue(Src);
}

void FBoolProperty::CopyCompleteValueToScriptVM(void* Dest, const void* Src) const {
    for (int32 i = 0; i < ArrayDim; i++)
        static_cast<bool*>(Dest)[i] = GetPropertyValue(static_cast<const uint8*>(Src) + i * ElementSize);
}

void FBoolProperty::CopySingleValueFromScriptVM(void* Dest, const void* Src) const {
    SetPropertyValue(Dest, *static_cast<const bool*>(Src));
}

void FBoolProperty::CopyCompleteValueFromScriptVM(void* Dest, const void* Src) const {
    for (int32 i = 0; i < ArrayDim; i++)
        SetPropertyValue(static_cast<uint8*>(Dest) + i * ElementSize, static_cast<const bool*>(Src)[i]);
}

void FBoolProperty::SerializeItem(FArchive& Ar, void* Value) const {
    uint8 ByteValue = 0;
    Ar << ByteValue;
    SetPropertyValue(Value, ByteValue != 0);
}

bool FObjectPropertyBase::SameType(const FProperty* Other) const {
    return FProperty::SameType(Other) && static_cast<const FObjectPropertyBase*>(Other)->PropertyClass == PropertyClass;
}

uint32 FObjectProperty::GetValueTypeHashInternal(const void* Src) const {
    UObject* Object = GetObjectPropertyValue(Src);
    return Object ? Object->GetUniqueID() * 2654435761u : 0;
}

void FWeakObjectProperty::SerializeItem(FArchive& Ar, void* Value) const {
    UObject* Object = nullptr;
    Ar << Object;
    SetObjectPropertyValue(Value, Object);
}

void FObjectProperty::SerializeItem(FArchive& Ar, void* Value) const {
    UObject* Object = nullptr;
    Ar << Object;
    SetObjectPropertyValue(Value, Object);
}

void FLazyObjectProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // FUniqueObjectGuid.
    FGuid Guid;
    Ar << Guid;
    static_cast<FWeakObjectPtr*>(Value)->Reset();
}

void FSoftObjectProperty::SerializeItem(FArchive& Ar, void* Value) const {
    FSoftObjectPtr& Ptr = *static_cast<FSoftObjectPtr*>(Value);
    Ptr.Reset();
    Ar << Ptr.ObjectID;
}

uint32 FInterfaceProperty::GetValueTypeHashInternal(const void* Src) const {
    UObject* Object = static_cast<const FScriptInterface*>(Src)->GetObject();
    return Object ? Object->GetUniqueID() * 2654435761u : 0;
}

void FInterfaceProperty::SerializeItem(FArchive& Ar, void* Value) const {
    UObject* Object = nullptr;
    Ar << Object;
    FScriptInterface& Interface = *static_cast<FScriptInterface*>(Value);
    Interface.SetObject(Object);
    Interface.SetInterface(Object);
}

uint32 FNameProperty::GetValueTypeHashInternal(const void* Src) const {
    return static_cast<uint32>(FNameHasher()(*static_cast<const FName*>(Src)));
}

void FNameProperty::SerializeItem(FArchive& Ar, void* Value) const { Ar << *static_cast<FName*>(Value); }

void FStrProperty::SerializeItem(FArchive& Ar, void* Value) const { Ar << *static_cast<FString*>(Value); }

bool FTextProperty::Identical(const void* A, const void* B, uint32) const {
    const FText& TextA = *static_cast<const FText*>(A);
    if (B == nullptr) return TextA.IsEmpty();
    const FText& TextB = *static_cast<const FText*>(B);
    // UE: identical text data, or both culture invariant (or both from string tables) with equal strings.
    if (TextA.IdenticalTo(TextB)) return true;
    return TextA.ToString().Equals(TextB.ToString());
}



void FTextProperty::SerializeItem(FArchive& Ar, void* Value) const { Ar << *static_cast<FText*>(Value); }

uint32 FDelegateProperty::GetValueTypeHashInternal(const void* Src) const {
    const FScriptDelegate& Delegate = *static_cast<const FScriptDelegate*>(Src);
    return static_cast<uint32>(FNameHasher()(Delegate.GetFunctionName()));
}

void FDelegateProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // TScriptDelegate: the object, then the function name.
    UObject* Object = nullptr;
    FName FunctionName;
    Ar << Object << FunctionName;
    FScriptDelegate& Delegate = *static_cast<FScriptDelegate*>(Value);
    Delegate.Unbind();
    if (Object != nullptr && !FunctionName.isNone()) Delegate.BindUFunction(Object, FunctionName);
}

void FMulticastInlineDelegateProperty::AddDelegate(FScriptDelegate ScriptDelegate, UObject*, void* PropertyValue) const {
    static_cast<FMulticastScriptDelegate*>(PropertyValue)->AddUnique(ScriptDelegate);
}

void FMulticastInlineDelegateProperty::RemoveDelegate(const FScriptDelegate& ScriptDelegate, UObject*, void* PropertyValue) const {
    static_cast<FMulticastScriptDelegate*>(PropertyValue)->Remove(ScriptDelegate);
}

void FMulticastInlineDelegateProperty::ClearDelegate(UObject*, void* PropertyValue) const {
    static_cast<FMulticastScriptDelegate*>(PropertyValue)->Clear();
}

void FMulticastInlineDelegateProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // The invocation list: a count, then each delegate's object and function name.
    FMulticastScriptDelegate& Multicast = *static_cast<FMulticastScriptDelegate*>(Value);
    Multicast.Clear();
    int32 Count = Ar.ReadCount(12);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        UObject* Object = nullptr;
        FName FunctionName;
        Ar << Object << FunctionName;
        if (Object == nullptr || FunctionName.isNone()) continue;
        FScriptDelegate Delegate;
        Delegate.BindUFunction(Object, FunctionName);
        Multicast.Add(Delegate);
    }
}

uint32 FFieldPathProperty::GetValueTypeHashInternal(const void* Src) const {
    FField* Field = static_cast<const FFieldPath*>(Src)->Get();
    return static_cast<uint32>(reinterpret_cast<uintptr_t>(Field) >> 4);
}

void FFieldPathProperty::SerializeItem(FArchive& Ar, void* Value) const {
    // The names, outermost property first, then the owning struct; the runtime keeps the innermost first.
    FFieldPath& Out = *static_cast<FFieldPath*>(Value);
    Out.Reset();
    TArray<FName> Names;
    Ar << Names;
    UObject* Owner = nullptr;
    Ar << Owner;
    for (int32 i = Names.Num() - 1; i >= 0; i--) Out.Path.Add(Names[i]);
    if (Owner != nullptr) Out.ResolvedOwner = Owner;
}

FDefaultConstructedPropertyElement::FDefaultConstructedPropertyElement(const FProperty* InProp) : Prop(InProp) {
    Obj = RuntimeMalloc(static_cast<std::size_t>(Prop->GetSize()), static_cast<std::size_t>(Prop->GetMinAlignment()));
    std::memset(Obj, 0, static_cast<std::size_t>(Prop->GetSize()));
    Prop->InitializeValue(Obj);
}

FDefaultConstructedPropertyElement::~FDefaultConstructedPropertyElement() {
    Prop->DestroyValue(Obj);
    RuntimeFree(Obj);
}

namespace {

struct FPropertyClassEntry {
    const char* Name;
    FFieldClass* (*Class)();
};

const FPropertyClassEntry PropertyClasses[] = {
    {"ByteProperty", &FByteProperty::StaticClass},
    {"Int8Property", &FInt8Property::StaticClass},
    {"Int16Property", &FInt16Property::StaticClass},
    {"IntProperty", &FIntProperty::StaticClass},
    {"Int64Property", &FInt64Property::StaticClass},
    {"UInt16Property", &FUInt16Property::StaticClass},
    {"UInt32Property", &FUInt32Property::StaticClass},
    {"UInt64Property", &FUInt64Property::StaticClass},
    {"FloatProperty", &FFloatProperty::StaticClass},
    {"DoubleProperty", &FDoubleProperty::StaticClass},
    {"LargeWorldCoordinatesRealProperty", &FDoubleProperty::StaticClass},
    {"BoolProperty", &FBoolProperty::StaticClass},
    {"ObjectProperty", &FObjectProperty::StaticClass},
    {"ObjectPtrProperty", &FObjectProperty::StaticClass},
    {"ClassProperty", &FClassProperty::StaticClass},
    {"ClassPtrProperty", &FClassProperty::StaticClass},
    {"WeakObjectProperty", &FWeakObjectProperty::StaticClass},
    {"LazyObjectProperty", &FLazyObjectProperty::StaticClass},
    {"SoftObjectProperty", &FSoftObjectProperty::StaticClass},
    {"AssetObjectProperty", &FSoftObjectProperty::StaticClass},
    {"SoftClassProperty", &FSoftClassProperty::StaticClass},
    {"InterfaceProperty", &FInterfaceProperty::StaticClass},
    {"NameProperty", &FNameProperty::StaticClass},
    {"StrProperty", &FStrProperty::StaticClass},
    {"TextProperty", &FTextProperty::StaticClass},
    {"StructProperty", &FStructProperty::StaticClass},
    {"ArrayProperty", &FArrayProperty::StaticClass},
    {"SetProperty", &FSetProperty::StaticClass},
    {"MapProperty", &FMapProperty::StaticClass},
    {"EnumProperty", &FEnumProperty::StaticClass},
    {"DelegateProperty", &FDelegateProperty::StaticClass},
    {"MulticastDelegateProperty", &FMulticastInlineDelegateProperty::StaticClass},
    {"MulticastInlineDelegateProperty", &FMulticastInlineDelegateProperty::StaticClass},
    {"MulticastSparseDelegateProperty", &FMulticastInlineDelegateProperty::StaticClass},
    {"FieldPathProperty", &FFieldPathProperty::StaticClass},
};

}

FFieldClass* FindPropertyClass(std::string_view Name) {
    for (const FPropertyClassEntry& Entry : PropertyClasses)
        if (Name == Entry.Name) return Entry.Class();
    return nullptr;
}

FProperty* ConstructProperty(FFieldClass* Class, FFieldVariant Owner, FName Name, EObjectFlags Flags) {
    if (Class == FByteProperty::StaticClass()) return new FByteProperty(Owner, Name, Flags);
    if (Class == FInt8Property::StaticClass()) return new FInt8Property(Owner, Name, Flags);
    if (Class == FInt16Property::StaticClass()) return new FInt16Property(Owner, Name, Flags);
    if (Class == FIntProperty::StaticClass()) return new FIntProperty(Owner, Name, Flags);
    if (Class == FInt64Property::StaticClass()) return new FInt64Property(Owner, Name, Flags);
    if (Class == FUInt16Property::StaticClass()) return new FUInt16Property(Owner, Name, Flags);
    if (Class == FUInt32Property::StaticClass()) return new FUInt32Property(Owner, Name, Flags);
    if (Class == FUInt64Property::StaticClass()) return new FUInt64Property(Owner, Name, Flags);
    if (Class == FFloatProperty::StaticClass()) return new FFloatProperty(Owner, Name, Flags);
    if (Class == FDoubleProperty::StaticClass()) return new FDoubleProperty(Owner, Name, Flags);
    if (Class == FBoolProperty::StaticClass()) return new FBoolProperty(Owner, Name, Flags);
    if (Class == FObjectProperty::StaticClass()) return new FObjectProperty(Owner, Name, Flags);
    if (Class == FClassProperty::StaticClass()) return new FClassProperty(Owner, Name, Flags);
    if (Class == FWeakObjectProperty::StaticClass()) return new FWeakObjectProperty(Owner, Name, Flags);
    if (Class == FLazyObjectProperty::StaticClass()) return new FLazyObjectProperty(Owner, Name, Flags);
    if (Class == FSoftObjectProperty::StaticClass()) return new FSoftObjectProperty(Owner, Name, Flags);
    if (Class == FSoftClassProperty::StaticClass()) return new FSoftClassProperty(Owner, Name, Flags);
    if (Class == FInterfaceProperty::StaticClass()) return new FInterfaceProperty(Owner, Name, Flags);
    if (Class == FNameProperty::StaticClass()) return new FNameProperty(Owner, Name, Flags);
    if (Class == FStrProperty::StaticClass()) return new FStrProperty(Owner, Name, Flags);
    if (Class == FTextProperty::StaticClass()) return new FTextProperty(Owner, Name, Flags);
    if (Class == FStructProperty::StaticClass()) return new FStructProperty(Owner, Name, Flags);
    if (Class == FArrayProperty::StaticClass()) return new FArrayProperty(Owner, Name, Flags);
    if (Class == FSetProperty::StaticClass()) return new FSetProperty(Owner, Name, Flags);
    if (Class == FMapProperty::StaticClass()) return new FMapProperty(Owner, Name, Flags);
    if (Class == FEnumProperty::StaticClass()) return new FEnumProperty(Owner, Name, Flags);
    if (Class == FDelegateProperty::StaticClass()) return new FDelegateProperty(Owner, Name, Flags);
    if (Class == FMulticastInlineDelegateProperty::StaticClass()) return new FMulticastInlineDelegateProperty(Owner, Name, Flags);
    if (Class == FFieldPathProperty::StaticClass()) return new FFieldPathProperty(Owner, Name, Flags);
    return nullptr;
}

}
