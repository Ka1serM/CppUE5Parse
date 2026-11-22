#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "CppUE5Parse/Package/Package.h"
#include "Engine/Source/Runtime/CoreUObject/CoreUObjectPrivate.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnknownProperties.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

thread_local const FOW2ObjectsArchive* GActiveOW2Archive = nullptr;

constexpr FName NAME_ArrayProperty("ArrayProperty");
constexpr FName NAME_StructProperty("StructProperty");

constexpr uint32 OW2ObjectIndexMask = 0xFFFFFF;
constexpr uint32 OW2SoftObjectPathType = 0xA9;

struct FTagSlot {
    FName Name;
    FPropertyTypeName Type;
    int32 ArrayDim = 1;
};

void AddTagSlot(std::vector<FTagSlot>& Slots, FName Name, const FPropertyTypeName& Type, int32 ArrayIndex) {
    const auto Found = std::ranges::find(Slots, Name, &FTagSlot::Name);
    if (Found != Slots.end()) {
        Found->ArrayDim = std::max(Found->ArrayDim, ArrayIndex + 1);
        return;
    }
    Slots.push_back(FTagSlot{Name, Type, ArrayIndex + 1});
}

void AppendTypeName(std::string& Out, const FPropertyTypeName& Type) {
    Out += Type.GetName().getText();
    const int32 Count = Type.GetParameterCount();
    if (Count == 0)
        return;
    Out += '(';
    for (int32 Index = 0; Index < Count; Index++) {
        if (Index > 0)
            Out += ',';
        AppendTypeName(Out, Type.GetParameter(Index));
    }
    Out += ')';
}

//! Lists with the same names and types share one transient struct, named after the hash of the list.
const UScriptStruct* GetOrCreateTagStruct(const std::vector<FTagSlot>& Slots) {
    std::string Signature;
    for (const FTagSlot& Slot : Slots) {
        Signature += Slot.Name.getText();
        Signature += ':';
        AppendTypeName(Signature, Slot.Type);
        Signature += '[' + std::to_string(Slot.ArrayDim) + "];";
    }
    char Buffer[48];
    std::snprintf(Buffer, sizeof(Buffer), "OW2PropertyData_%llx", static_cast<unsigned long long>(std::hash<std::string>{}(Signature)));
    const FName StructName = MakeName(std::string_view(Buffer));
    std::lock_guard Lock(GetLoaderMutex());
    if (UObject* Existing = StaticFindObjectFast(UScriptStruct::StaticClass(), GetTransientPackage(), StructName, true))
        return Cast<UScriptStruct>(Existing);
    auto* Struct = NewObject<UScriptStruct>(GetTransientPackage(), UScriptStruct::StaticClass(), StructName, RF_Standalone | RF_Transient);
    for (const FTagSlot& Slot : Slots) {
        FProperty* Property = NewPropertyFromTypeName(Slot.Name, Slot.Type, FFieldVariant(Struct));
        if (Property == nullptr) {
            ReportLoadProblem("load.unknown_type", FString(Slot.Name.toString()));
            continue;
        }
        Property->ArrayDim = Slot.ArrayDim;
        Struct->AddCppProperty(Property);
        Struct->OwnUnversionedSlots.Add(UStruct::FUnversionedSlot{Property, 0});
    }
    Struct->Link();
    return Struct;
}

bool CollectTagSlots(FArchive& Ar, std::vector<FTagSlot>& Slots) {
    while (!Ar.IsError()) {
        FPropertyTag Tag;
        Ar << Tag;
        if (Ar.IsError() || Tag.Name.isNone())
            break;
        const int64 Start = Ar.Tell();
        const FPropertyTypeName Type = GetPropertyTagTypeName(Ar, Tag);
        AddTagSlot(Slots, Tag.Name, Type, Tag.ArrayIndex);
        Ar.Seek(Start);
        Ar.SkipBytes(Tag.Size);
    }
    return !Ar.IsError();
}

//! The array tags of The Outer Worlds 2 name no inner type, so CUE4Parse knows it only for these properties.
FName GetOW2ArrayInnerType(FName Name) {
    if (Name.getText() == "WaveIndexes")
        return FName("IntProperty");
    if (Name.getText() == "CoverObject")
        return FName("SoftObjectProperty");
    return FName();
}

struct FOW2PropertyTag {
    FName Name;
    FPropertyTypeName Type;
    bool bHasVersion = false;
    bool bIsDefault = false;
    TArray<uint8> Data;
    FPropertyDataObjectContainer Objects;
};

std::span<const std::byte> AsBytes(const TArray<uint8>& Data) {
    return std::as_bytes(std::span<const uint8>(Data.GetData(), static_cast<std::size_t>(Data.Num())));
}

}

FArchive& operator<<(FArchive& Ar, FPropertyDataObjectContainer& Container) {
    const int32 Count = Ar.ReadCount(4);
    Container.ObjectStore.Empty(Count);
    FLinkerLoad* Linker = Ar.GetLinker();
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        FPackageIndex Object;
        Ar << Object;
        Container.ObjectStore.Add(Linker != nullptr && !Ar.IsError() ? Linker->IndexToObject(Object) : nullptr);
    }
    return Ar << Container.SoftObjectPathStore;
}

FOW2ObjectsArchive::FOW2ObjectsArchive(std::span<const std::byte> InBytes, FArchive& Outer, const FPropertyDataObjectContainer& InObjects, bool bInHasVersion)
    : FMemoryReaderView(InBytes), Linker(Outer.GetLinker()), Objects(InObjects), bHasVersion(bInHasVersion), Previous(GActiveOW2Archive) {
    CopyStateFrom(Outer);
    SetUseUnversionedPropertySerialization(false);
    FPackageFileVersion Version{522, 1010};
    if (bHasVersion) {
        Version.fileVersionUE4 = Read<int32>();
        Version.fileVersionUE5 = Read<int32>();
    }
    SetUEVer(Version);
    GActiveOW2Archive = this;
}

FOW2ObjectsArchive::~FOW2ObjectsArchive() {
    GActiveOW2Archive = Previous;
}

const FOW2ObjectsArchive* FOW2ObjectsArchive::Find(const FArchive& Ar) {
    for (const FOW2ObjectsArchive* Archive = GActiveOW2Archive; Archive != nullptr; Archive = Archive->Previous)
        if (Archive == &Ar)
            return Archive;
    return nullptr;
}

FArchive& FOW2ObjectsArchive::operator<<(FName& Value) {
    FString Text;
    *this << Text;
    Value = IsError() ? FName() : MakeName(Text.ToUtf8());
    return *this;
}

FArchive& FOW2ObjectsArchive::operator<<(UObject*& Value) {
    const uint32 Data = Read<uint32>();
    const int32 Index = static_cast<int32>(Data & OW2ObjectIndexMask);
    Value = nullptr;
    if (IsError())
        return *this;
    if (!Objects.ObjectStore.IsValidIndex(Index)) {
        SetError(ErrorCode::InvalidData, "FOW2ObjectsArchive.UObject");
        return *this;
    }
    Value = Objects.ObjectStore[Index];
    return *this;
}

//! A soft path is the first value of type 0xA9; the values before it are skipped.
FArchive& FOW2ObjectsArchive::operator<<(FSoftObjectPath& Value) {
    Value.Reset();
    while (!IsError()) {
        const uint32 Data = Read<uint32>();
        if (IsError() || (Data >> 24) != OW2SoftObjectPathType)
            continue;
        const int32 Index = static_cast<int32>(Data & OW2ObjectIndexMask);
        if (!Objects.SoftObjectPathStore.IsValidIndex(Index)) {
            SetError(ErrorCode::InvalidData, "FOW2ObjectsArchive.FSoftObjectPath");
            break;
        }
        Value = Objects.SoftObjectPathStore[Index];
        break;
    }
    return *this;
}

FArchive& FOW2ObjectsArchive::operator<<(FSoftObjectPtr& Value) {
    Value.Reset();
    return *this << Value.ObjectID;
}

FArchive& FOW2ObjectsArchive::operator<<(FWeakObjectPtr& Value) {
    UObject* Object = nullptr;
    *this << Object;
    Value = Object;
    return *this;
}

void SerializeTaggedPropertiesWithoutStruct(FArchive& Ar, FInstancedStruct& Value) {
    const int64 Start = Ar.Tell();
    std::vector<FTagSlot> Slots;
    if (!CollectTagSlots(Ar, Slots))
        return;
    Ar.Seek(Start);
    Value.InitializeAs(GetOrCreateTagStruct(Slots));
    Value.GetScriptStruct()->SerializeTaggedProperties(Ar, Value.GetMutableMemory());
}

void SerializeStructFallback(FArchive& Ar, FName StructName, FInstancedStruct& Value) {
    if (const UScriptStruct* Struct = FindNativeStruct(StructName)) {
        Value.InitializeAs(Struct);
        Struct->SerializeItem(Ar, Value.GetMutableMemory());
        return;
    }
    if (Ar.UseUnversionedPropertySerialization()) {
        Ar.SetError(ErrorCode::InvalidData, "SerializeStructFallback");
        return;
    }
    SerializeTaggedPropertiesWithoutStruct(Ar, Value);
}

void SerializeStructByObject(FArchive& Ar, const UObject* StructObject, FInstancedStruct& Value) {
    if (StructObject == nullptr)
        return;
    if (const UScriptStruct* Struct = Cast<UScriptStruct>(StructObject)) {
        Value.InitializeAs(Struct);
        Struct->SerializeItem(Ar, Value.GetMutableMemory());
        return;
    }
    SerializeStructFallback(Ar, StructObject->GetFName(), Value);
}

void SerializeStructByObject(FArchive& Ar, FPackageIndex StructIndex, FInstancedStruct& Value) {
    FLinkerLoad* Linker = Ar.GetLinker();
    if (StructIndex.IsNull() || Linker == nullptr)
        return;
    if (const UObject* Object = Linker->IndexToObject(StructIndex)) {
        SerializeStructByObject(Ar, Object, Value);
        return;
    }
    const FName Name = Linker->GetPackage().resolve(StructIndex.Index).getName();
    if (Name.isNone()) {
        Ar.SetError(ErrorCode::InvalidData, "SerializeStructByObject");
        return;
    }
    SerializeStructFallback(Ar, Name, Value);
}

void SerializeOW2PropertyTags(FArchive& Ar, FInstancedStruct& Value) {
    const int32 Count = Ar.ReadCount(8);
    std::vector<FOW2PropertyTag> Tags(static_cast<std::size_t>(Count));
    std::vector<FTagSlot> Slots;
    for (FOW2PropertyTag& Tag : Tags) {
        Ar << Tag.Name;
        if (Ar.IsError())
            return;
        if (Tag.Name.isNone())
            continue;
        FName Type;
        Ar << Tag.bHasVersion << Type << Tag.bIsDefault << Tag.Data << Tag.Objects;
        if (Ar.IsError())
            return;
        if (Type == NAME_StructProperty) {
            ReportLoadProblem("load.unknown_type", FString(Tag.Name.toString()) + FString(" of type StructProperty"));
            continue;
        }
        Tag.Type.AddNode(Type, Type == NAME_ArrayProperty ? 1 : 0);
        if (Type == NAME_ArrayProperty) {
            const FName InnerType = GetOW2ArrayInnerType(Tag.Name);
            if (InnerType.isNone()) {
                ReportLoadProblem("load.unknown_type", FString(Tag.Name.toString()) + FString(" of type ArrayProperty"));
                continue;
            }
            Tag.Type.AddNode(InnerType, 0);
        }
        AddTagSlot(Slots, Tag.Name, Tag.Type, 0);
    }
    Value.InitializeAs(GetOrCreateTagStruct(Slots));
    const UScriptStruct* Struct = Value.GetScriptStruct();
    for (const FOW2PropertyTag& Tag : Tags) {
        const FProperty* Property = Tag.Name.isNone() || Tag.Type.GetName().isNone() ? nullptr : Struct->FindPropertyByName(Tag.Name);
        if (Property == nullptr)
            continue;
        void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Value.GetMutableMemory());
        FOW2ObjectsArchive ObjectAr(AsBytes(Tag.Data), Ar, Tag.Objects, Tag.bHasVersion);
        ObjectAr.Read<int32>();
        if (Tag.Type.GetName() == NAME_ArrayProperty) {
            FString InnerType;
            ObjectAr << InnerType;
            ObjectAr.Read<int32>();
        }
        if (const FBoolProperty* Bool = CastField<FBoolProperty>(Property))
            Bool->SetPropertyValue(PropertyValue, ObjectAr.Read<uint8>() != 0);
        else
            Property->SerializeItem(ObjectAr, PropertyValue);
        if (ObjectAr.IsError())
            ReportLoadProblem("load.unknown_value", FString(Tag.Name.toString()) + FString(" of The Outer Worlds 2 property data"));
    }
}

bool FPropertyDataWithObjects::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_OuterWorlds2)
        return false;
    const FOW2ObjectsArchive* OuterOW2 = FOW2ObjectsArchive::Find(Ar);
    bool bHasVersion = false;
    if (OuterOW2 == nullptr || OuterOW2->HasVersion())
        Ar << bHasVersion;
    TArray<uint8> Data;
    FPropertyDataObjectContainer Objects;
    Ar << Data << Objects;
    if (Ar.IsError())
        return true;
    FOW2ObjectsArchive ObjectAr(AsBytes(Data), Ar, Objects, bHasVersion);
    SerializeTaggedPropertiesWithoutStruct(ObjectAr, Properties);
    if (ObjectAr.IsError())
        ReportLoadProblem("load.unknown_value", FString("PropertyDataWithObjects of The Outer Worlds 2"));
    return true;
}

}
