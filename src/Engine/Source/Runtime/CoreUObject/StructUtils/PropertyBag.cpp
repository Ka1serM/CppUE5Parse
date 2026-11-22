#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>

#include "Engine/Source/Runtime/CoreUObject/CoreUObjectPrivate.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

template <class T>
void AppendBytes(std::string& Bytes, const T& Value) {
    Bytes.append(reinterpret_cast<const char*>(&Value), sizeof(T));
}

uint64 CalcPropertyDescArrayHash(const TArray<FPropertyBagPropertyDesc>& Descs) {
    std::string Bytes;
    for (const FPropertyBagPropertyDesc& Desc : Descs) {
        AppendBytes(Bytes, Desc.ValueTypeObject);
        AppendBytes(Bytes, Desc.ID);
        AppendBytes(Bytes, Desc.Name.getHash());
        AppendBytes(Bytes, Desc.Name.getNumber());
        AppendBytes(Bytes, Desc.ValueType);
        AppendBytes(Bytes, Desc.ContainerTypes.NumContainers);
        for (uint8 Index = 0; Index < Desc.ContainerTypes.NumContainers; Index++)
            AppendBytes(Bytes, Desc.ContainerTypes.Types[Index]);
        AppendBytes(Bytes, Desc.PropertyFlags);
        AppendBytes(Bytes, Desc.KeyType);
        AppendBytes(Bytes, Desc.KeyTypeObject);
    }
    return cityHash64WithSeed(Bytes, 0);
}

FNumericProperty* CreateIntegerProperty(EPropertyBagPropertyType Type, FFieldVariant Owner, FName Name) {
    switch (Type) {
    case EPropertyBagPropertyType::Int8:
        return new FInt8Property(Owner, Name);
    case EPropertyBagPropertyType::Byte:
        return new FByteProperty(Owner, Name);
    case EPropertyBagPropertyType::Int16:
        return new FInt16Property(Owner, Name);
    case EPropertyBagPropertyType::UInt16:
        return new FUInt16Property(Owner, Name);
    case EPropertyBagPropertyType::Int32:
        return new FIntProperty(Owner, Name);
    case EPropertyBagPropertyType::UInt32:
        return new FUInt32Property(Owner, Name);
    case EPropertyBagPropertyType::Int64:
        return new FInt64Property(Owner, Name);
    case EPropertyBagPropertyType::UInt64:
        return new FUInt64Property(Owner, Name);
    default:
        return nullptr;
    }
}

//! The port's UEnum does not keep the underlying type, so an enum class reads through a byte as UE 5.4 and older do.
FProperty* CreateEnumProperty(UEnum* Enum, FFieldVariant Owner, FName Name) {
    if (Enum == nullptr)
        return new FByteProperty(Owner, Name);
    if (Enum->CppForm != ECppForm::EnumClass) {
        auto* Property = new FByteProperty(Owner, Name);
        Property->Enum = Enum;
        return Property;
    }
    auto* Property = new FEnumProperty(Owner, Name);
    Property->Enum = Enum;
    Property->UnderlyingProp = new FByteProperty(Property, FName("UnderlyingType"));
    return Property;
}

UClass* GetObjectTypeClass(UObject* ValueTypeObject) {
    UClass* Class = Cast<UClass>(ValueTypeObject);
    return Class != nullptr ? Class : UObject::StaticClass();
}

FProperty* CreatePropertyFromDesc(const FPropertyBagPropertyDesc& Desc, FFieldVariant Owner);

FProperty* CreateContainerPropertyFromDesc(const FPropertyBagPropertyDesc& Desc, FFieldVariant Owner) {
    std::unique_ptr<FProperty> Outer;
    FProperty** ValuePropertyPtr = nullptr;
    FFieldVariant PropertyOwner = Owner;
    for (uint8 Index = 0; Index < Desc.ContainerTypes.NumContainers; Index++) {
        FProperty* Container = nullptr;
        FProperty** Next = nullptr;
        switch (Desc.ContainerTypes.Types[Index]) {
        case EPropertyBagContainerType::Array: {
            auto* Array = new FArrayProperty(PropertyOwner, Desc.Name);
            Container = Array;
            Next = &Array->Inner;
            break;
        }
        case EPropertyBagContainerType::Set: {
            auto* Set = new FSetProperty(PropertyOwner, Desc.Name);
            Container = Set;
            Next = &Set->ElementProp;
            break;
        }
        case EPropertyBagContainerType::Map: {
            auto* Map = new FMapProperty(PropertyOwner, Desc.Name);
            FPropertyBagPropertyDesc KeyDesc;
            KeyDesc.ValueType = Desc.KeyType;
            KeyDesc.ValueTypeObject = Desc.KeyTypeObject;
            KeyDesc.Name = Desc.Name;
            Map->KeyProp = CreatePropertyFromDesc(KeyDesc, Map);
            Container = Map;
            Next = &Map->ValueProp;
            break;
        }
        default:
            return nullptr;
        }
        if (ValuePropertyPtr == nullptr)
            Outer.reset(Container);
        else
            *ValuePropertyPtr = Container;
        ValuePropertyPtr = Next;
        PropertyOwner = Container;
    }
    FPropertyBagPropertyDesc InnerDesc = Desc;
    InnerDesc.ContainerTypes = FPropertyBagContainerTypes();
    *ValuePropertyPtr = CreatePropertyFromDesc(InnerDesc, PropertyOwner);
    if (*ValuePropertyPtr == nullptr)
        return nullptr;
    return Outer.release();
}

FProperty* CreatePropertyFromDesc(const FPropertyBagPropertyDesc& Desc, FFieldVariant Owner) {
    if (Desc.ContainerTypes.NumContainers > 0)
        return CreateContainerPropertyFromDesc(Desc, Owner);
    switch (Desc.ValueType) {
    case EPropertyBagPropertyType::Bool: {
        auto* Property = new FBoolProperty(Owner, Desc.Name);
        Property->SetBoolSize(sizeof(bool), true);
        return Property;
    }
    case EPropertyBagPropertyType::Int8:
    case EPropertyBagPropertyType::Byte:
    case EPropertyBagPropertyType::Int16:
    case EPropertyBagPropertyType::UInt16:
    case EPropertyBagPropertyType::Int32:
    case EPropertyBagPropertyType::UInt32:
    case EPropertyBagPropertyType::Int64:
    case EPropertyBagPropertyType::UInt64:
        return CreateIntegerProperty(Desc.ValueType, Owner, Desc.Name);
    case EPropertyBagPropertyType::Float:
        return new FFloatProperty(Owner, Desc.Name);
    case EPropertyBagPropertyType::Double:
        return new FDoubleProperty(Owner, Desc.Name);
    case EPropertyBagPropertyType::Name:
        return new FNameProperty(Owner, Desc.Name);
    case EPropertyBagPropertyType::String:
        return new FStrProperty(Owner, Desc.Name);
    case EPropertyBagPropertyType::Text:
        return new FTextProperty(Owner, Desc.Name);
    case EPropertyBagPropertyType::Enum:
        return CreateEnumProperty(Cast<UEnum>(Desc.ValueTypeObject), Owner, Desc.Name);
    case EPropertyBagPropertyType::Struct: {
        auto* Property = new FStructProperty(Owner, Desc.Name);
        UScriptStruct* Struct = Cast<UScriptStruct>(Desc.ValueTypeObject);
        Property->Struct = Struct != nullptr ? Struct : GetUnknownStruct(FName("PropertyBagMissingStruct"));
        return Property;
    }
    case EPropertyBagPropertyType::Object: {
        auto* Property = new FObjectProperty(Owner, Desc.Name);
        Property->PropertyClass = GetObjectTypeClass(Desc.ValueTypeObject);
        return Property;
    }
    case EPropertyBagPropertyType::SoftObject: {
        auto* Property = new FSoftObjectProperty(Owner, Desc.Name);
        Property->PropertyClass = GetObjectTypeClass(Desc.ValueTypeObject);
        return Property;
    }
    case EPropertyBagPropertyType::Class: {
        auto* Property = new FClassProperty(Owner, Desc.Name);
        Property->MetaClass = GetObjectTypeClass(Desc.ValueTypeObject);
        Property->PropertyClass = UClass::StaticClass();
        return Property;
    }
    case EPropertyBagPropertyType::SoftClass: {
        auto* Property = new FSoftClassProperty(Owner, Desc.Name);
        Property->MetaClass = GetObjectTypeClass(Desc.ValueTypeObject);
        Property->PropertyClass = UClass::StaticClass();
        return Property;
    }
    default:
        return nullptr;
    }
}

//! Before NestedContainerTypes a desc saves one container type, and None means no container.
void SerializeContainerTypes(FArchive& Ar, FPropertyBagContainerTypes& ContainerTypes) {
    if (Ar.CustomVerAtLeast(FPropertyBagCustomVersion::GUID, FPropertyBagCustomVersion::NestedContainerTypes, EGame::GAME_UE5_3)) {
        ContainerTypes.Serialize(Ar);
        return;
    }
    if (!Ar.CustomVerAtLeast(FPropertyBagCustomVersion::GUID, FPropertyBagCustomVersion::ContainerTypes, EGame::GAME_UE5_1))
        return;
    EPropertyBagContainerType ContainerType = EPropertyBagContainerType::None;
    Ar << ContainerType;
    if (ContainerType != EPropertyBagContainerType::None)
        ContainerTypes.Add(ContainerType);
}

TArray<FPropertyBagPropertyDesc> RemoveDuplicateNames(const TArray<FPropertyBagPropertyDesc>& Descs) {
    TArray<FPropertyBagPropertyDesc> Unique;
    for (const FPropertyBagPropertyDesc& Desc : Descs) {
        bool bFound = false;
        for (const FPropertyBagPropertyDesc& Existing : Unique)
            bFound |= Existing.Name == Desc.Name;
        if (!bFound)
            Unique.Add(Desc);
    }
    return Unique;
}

}

void FPropertyBagContainerTypes::Add(EPropertyBagContainerType Type) {
    if (NumContainers < MaxNestedTypes)
        Types[NumContainers++] = Type;
}

void FPropertyBagContainerTypes::Serialize(FArchive& Ar) {
    Ar << NumContainers;
    if (NumContainers > MaxNestedTypes) {
        Ar.SetError(ErrorCode::InvalidData, "FPropertyBagContainerTypes.NumContainers");
        return;
    }
    for (uint8 Index = 0; Index < NumContainers; Index++)
        Ar << Types[Index];
}

void FPropertyBagPropertyDesc::Serialize(FArchive& Ar) {
    Ar << ValueTypeObject;
    Ar << ID;
    Ar << Name;
    Ar << ValueType;
    SerializeContainerTypes(Ar, ContainerTypes);
    bool bHasMetaData = false;
    Ar << bHasMetaData;
    if (bHasMetaData) {
        Ar << MetaData;
        if (Ar.CustomVerAtLeast(FPropertyBagCustomVersion::GUID, FPropertyBagCustomVersion::MetaClass, EGame::GAME_UE5_4))
            Ar << MetaClass;
    }
    if (Ar.CustomVerAtLeast(FPropertyBagCustomVersion::GUID, FPropertyBagCustomVersion::PropertyFlags, EGame::GAME_UE5_8))
        Ar << PropertyFlags;
    if (Ar.CustomVerAtLeast(FPropertyBagCustomVersion::GUID, FPropertyBagCustomVersion::KeyTypes, EGame::GAME_UE5_8)) {
        Ar << KeyType;
        Ar << KeyTypeObject;
    }
}

IMPLEMENT_CLASS(UPropertyBag, "PropertyBag")

const UPropertyBag* GetOrCreatePropertyBag(const TArray<FPropertyBagPropertyDesc>& PropertyDescs) {
    char Buffer[32];
    std::snprintf(Buffer, sizeof(Buffer), "PropertyBag_%llx", static_cast<unsigned long long>(CalcPropertyDescArrayHash(PropertyDescs)));
    const FName BagName = MakeName(std::string_view(Buffer));
    std::lock_guard Lock(GetLoaderMutex());
    if (UObject* Existing = StaticFindObjectFast(UPropertyBag::StaticClass(), GetTransientPackage(), BagName, true))
        return Cast<UPropertyBag>(Existing);
    auto* Bag = NewObject<UPropertyBag>(GetTransientPackage(), UPropertyBag::StaticClass(), BagName, RF_Standalone | RF_Transient);
    Bag->PropertyDescs = RemoveDuplicateNames(PropertyDescs);
    for (const FPropertyBagPropertyDesc& Desc : Bag->PropertyDescs) {
        FProperty* Property = CreatePropertyFromDesc(Desc, Bag);
        if (Property == nullptr)
            continue;
        Property->PropertyFlags = static_cast<EPropertyFlags>(Property->PropertyFlags | Desc.PropertyFlags);
        Bag->AddCppProperty(Property);
        Bag->OwnUnversionedSlots.Add(UStruct::FUnversionedSlot{Property, 0});
    }
    Bag->Link();
    return Bag;
}

const UPropertyBag* FInstancedPropertyBag::GetPropertyBagStruct() const {
    return Value.IsValid() ? Cast<UPropertyBag>(Value.GetScriptStruct()) : nullptr;
}

//! UE 5.0 has no property bag; before ContainerTypes a version byte decides whether the serial size is saved.
bool FInstancedPropertyBag::Serialize(FArchive& Ar) {
    uint8 Version = 1;
    if (!Ar.CustomVerAtLeast(FPropertyBagCustomVersion::GUID, FPropertyBagCustomVersion::ContainerTypes, EGame::GAME_UE5_1))
        Ar << Version;
    bool bHasData = false;
    Ar << bHasData;
    if (Ar.IsError() || !bHasData) {
        Value.Reset();
        return true;
    }
    TArray<FPropertyBagPropertyDesc> PropertyDescs;
    const int32 Num = Ar.ReadCount(33);
    for (int32 Index = 0; Index < Num && !Ar.IsError(); Index++)
        PropertyDescs[PropertyDescs.AddDefaulted()].Serialize(Ar);
    int32 SerialSize = 0;
    if (Version >= 1)
        Ar << SerialSize;
    if (Ar.IsError())
        return true;
    Value.InitializeAs(GetOrCreatePropertyBag(PropertyDescs));
    if (Version >= 1) {
        SerializeStructInstance(Ar, Value.GetScriptStruct(), Value.GetMutableMemory(), SerialSize);
        return true;
    }
    Value.GetScriptStruct()->SerializeItem(Ar, Value.GetMutableMemory());
    return true;
}

}
