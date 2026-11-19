#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealTypePrivate.h"

#include <algorithm>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

IMPLEMENT_CLASS(UProperty, "Property")
IMPLEMENT_CLASS(UNumericProperty, "NumericProperty")
IMPLEMENT_CLASS(UByteProperty, "ByteProperty")
IMPLEMENT_CLASS(UInt8Property, "Int8Property")
IMPLEMENT_CLASS(UInt16Property, "Int16Property")
IMPLEMENT_CLASS(UIntProperty, "IntProperty")
IMPLEMENT_CLASS(UInt64Property, "Int64Property")
IMPLEMENT_CLASS(UUInt16Property, "UInt16Property")
IMPLEMENT_CLASS(UUInt32Property, "UInt32Property")
IMPLEMENT_CLASS(UUInt64Property, "UInt64Property")
IMPLEMENT_CLASS(UFloatProperty, "FloatProperty")
IMPLEMENT_CLASS(UDoubleProperty, "DoubleProperty")
IMPLEMENT_CLASS(UBoolProperty, "BoolProperty")
IMPLEMENT_CLASS(UObjectPropertyBase, "ObjectPropertyBase")
IMPLEMENT_CLASS(UObjectProperty, "ObjectProperty")
IMPLEMENT_CLASS(UWeakObjectProperty, "WeakObjectProperty")
IMPLEMENT_CLASS(ULazyObjectProperty, "LazyObjectProperty")
IMPLEMENT_CLASS(USoftObjectProperty, "SoftObjectProperty")
IMPLEMENT_CLASS(UClassProperty, "ClassProperty")
IMPLEMENT_CLASS(USoftClassProperty, "SoftClassProperty")
IMPLEMENT_CLASS(UInterfaceProperty, "InterfaceProperty")
IMPLEMENT_CLASS(UNameProperty, "NameProperty")
IMPLEMENT_CLASS(UStrProperty, "StrProperty")
IMPLEMENT_CLASS(UTextProperty, "TextProperty")
IMPLEMENT_CLASS(UArrayProperty, "ArrayProperty")
IMPLEMENT_CLASS(UMapProperty, "MapProperty")
IMPLEMENT_CLASS(USetProperty, "SetProperty")
IMPLEMENT_CLASS(UStructProperty, "StructProperty")
IMPLEMENT_CLASS(UDelegateProperty, "DelegateProperty")
IMPLEMENT_CLASS(UMulticastDelegateProperty, "MulticastDelegateProperty")
IMPLEMENT_CLASS(UMulticastInlineDelegateProperty, "MulticastInlineDelegateProperty")
IMPLEMENT_CLASS(UMulticastSparseDelegateProperty, "MulticastSparseDelegateProperty")
IMPLEMENT_CLASS(UEnumProperty, "EnumProperty")

namespace {

template <class T>
T* SerializeObject(FArchive& Ar) {
    UObject* Object = nullptr;
    Ar << Object;
    return Cast<T>(Object);
}

}

void UProperty::Serialize(FArchive& Ar) {
    UField::Serialize(Ar);
    if (Ar.IsError())
        return;
    uint64 SaveFlags = 0;
    Ar << ArrayDim << SaveFlags << RepNotifyFunc;
    PropertyFlags = EPropertyFlags(SaveFlags);
    if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::PropertiesSerializeRepCondition, EGame::GAME_UE4_22))
        Ar << BlueprintReplicationCondition;
}

void UByteProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    Enum = SerializeObject<UEnum>(Ar);
}

void UBoolProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    uint8 NativeBool = 0;
    Ar << BoolSize << NativeBool;
    bNativeBool = NativeBool != 0;
}

void UObjectPropertyBase::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    PropertyClass = SerializeObject<UClass>(Ar);
}

void UClassProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    MetaClass = SerializeObject<UClass>(Ar);
}

void USoftClassProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    MetaClass = SerializeObject<UClass>(Ar);
}

void UInterfaceProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    InterfaceClass = SerializeObject<UClass>(Ar);
}

void UArrayProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    Inner = SerializeObject<UProperty>(Ar);
}

void UMapProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    KeyProp = SerializeObject<UProperty>(Ar);
    ValueProp = SerializeObject<UProperty>(Ar);
}

void USetProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    ElementProp = SerializeObject<UProperty>(Ar);
}

void UStructProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    Struct = SerializeObject<UScriptStruct>(Ar);
}

void UDelegateProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    SignatureFunction = SerializeObject<UFunction>(Ar);
}

void UMulticastDelegateProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    SignatureFunction = SerializeObject<UFunction>(Ar);
}

void UEnumProperty::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    Enum = SerializeObject<UEnum>(Ar);
    UnderlyingProp = SerializeObject<UNumericProperty>(Ar);
}

namespace {

FProperty* CreateInner(UProperty* InField) {
    return InField ? CreateFromUField(InField) : nullptr;
}

bool ConvertTypeData(UProperty* InField, FProperty* Property) {
    if (auto* Bool = CastField<FBoolProperty>(Property)) {
        const auto* Legacy = static_cast<UBoolProperty*>(InField);
        Bool->SetBoolSize(Legacy->BoolSize, Legacy->bNativeBool);
        return true;
    }
    if (auto* Byte = CastField<FByteProperty>(Property)) {
        Byte->Enum = static_cast<UByteProperty*>(InField)->Enum;
        return true;
    }
    if (auto* Enum = CastField<FEnumProperty>(Property)) {
        auto* Legacy = static_cast<UEnumProperty*>(InField);
        Enum->Enum = Legacy->Enum;
        Enum->UnderlyingProp = CastField<FNumericProperty>(CreateInner(Legacy->UnderlyingProp));
        return Enum->UnderlyingProp != nullptr;
    }
    if (auto* Struct = CastField<FStructProperty>(Property)) {
        Struct->Struct = static_cast<UStructProperty*>(InField)->Struct;
        if (Struct->Struct != nullptr && Struct->Struct->IsLinked())
            return true;
        ReportLoadProblem("load.property_struct", InField->GetName() + FString(": ") + (Struct->Struct ? Struct->Struct->GetFullName() : FString("null")));
        return false;
    }
    if (auto* Class = CastField<FClassProperty>(Property)) {
        const auto* Legacy = static_cast<UClassProperty*>(InField);
        Class->PropertyClass = Legacy->PropertyClass;
        Class->MetaClass = Legacy->MetaClass;
        return true;
    }
    if (auto* SoftClass = CastField<FSoftClassProperty>(Property)) {
        const auto* Legacy = static_cast<USoftClassProperty*>(InField);
        SoftClass->PropertyClass = Legacy->PropertyClass;
        SoftClass->MetaClass = Legacy->MetaClass;
        return true;
    }
    if (auto* Object = CastField<FObjectPropertyBase>(Property)) {
        Object->PropertyClass = static_cast<UObjectPropertyBase*>(InField)->PropertyClass;
        return true;
    }
    if (auto* Interface = CastField<FInterfaceProperty>(Property)) {
        Interface->InterfaceClass = static_cast<UInterfaceProperty*>(InField)->InterfaceClass;
        return true;
    }
    if (auto* Delegate = CastField<FDelegateProperty>(Property)) {
        Delegate->SignatureFunction = static_cast<UDelegateProperty*>(InField)->SignatureFunction;
        return true;
    }
    if (auto* Multicast = CastField<FMulticastDelegateProperty>(Property)) {
        Multicast->SignatureFunction = static_cast<UMulticastDelegateProperty*>(InField)->SignatureFunction;
        return true;
    }
    if (auto* Array = CastField<FArrayProperty>(Property)) {
        Array->Inner = CreateInner(static_cast<UArrayProperty*>(InField)->Inner);
        return Array->Inner != nullptr;
    }
    if (auto* Set = CastField<FSetProperty>(Property)) {
        Set->ElementProp = CreateInner(static_cast<USetProperty*>(InField)->ElementProp);
        return Set->ElementProp != nullptr;
    }
    if (auto* Map = CastField<FMapProperty>(Property)) {
        const auto* Legacy = static_cast<UMapProperty*>(InField);
        Map->KeyProp = CreateInner(Legacy->KeyProp);
        Map->ValueProp = CreateInner(Legacy->ValueProp);
        return Map->KeyProp != nullptr && Map->ValueProp != nullptr;
    }
    return true;
}

}

FProperty* CreateFromUField(UProperty* InField) {
    if (FProperty* Existing = InField->GetAssociatedFField())
        return Existing;
    FFieldVariant Owner = InField->GetOuter();
    if (auto* OuterProperty = Cast<UProperty>(InField->GetOuter())) {
        FProperty* NewOwnerField = CreateFromUField(OuterProperty);
        if (NewOwnerField == nullptr)
            return nullptr;
        if (FProperty* Existing = InField->GetAssociatedFField())
            return Existing;
        Owner = NewOwnerField;
    }
    FFieldClass* FieldClass = FindPropertyClass(InField->GetClass()->GetFName().getText());
    if (FieldClass == nullptr) {
        ReportLoadProblem("load.property_class", InField->GetClass()->GetName());
        return nullptr;
    }
    const EObjectFlags Flags = EObjectFlags(InField->GetFlags() & ~uint32(RF_MarkAsNative | RF_NeedLoad | RF_WasLoaded));
    FProperty* Property = ConstructProperty(FieldClass, Owner, InField->GetFName(), Flags);
    InField->SetAssociatedFField(Property);
    Property->ArrayDim = std::max<int32>(InField->ArrayDim, 1);
    Property->PropertyFlags = InField->PropertyFlags;
    Property->RepNotifyFunc = InField->RepNotifyFunc;
    Property->BlueprintReplicationCondition = InField->BlueprintReplicationCondition;
    if (ConvertTypeData(InField, Property))
        return Property;
    ReportLoadProblem("load.property_type", InField->GetName() + FString(" (") + InField->GetClass()->GetName() + FString(")"));
    InField->SetAssociatedFField(nullptr);
    delete Property;
    return nullptr;
}

}
