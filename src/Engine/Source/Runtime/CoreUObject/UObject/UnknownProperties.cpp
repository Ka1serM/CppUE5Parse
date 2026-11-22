#include "Engine/Source/Runtime/CoreUObject/UObject/UnknownProperties.h"

#include <algorithm>

#include "CppUE5Parse/GameTypes/PropertyTypeGuesses.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

constexpr FName NAME_BoolProperty("BoolProperty");
constexpr FName NAME_ByteProperty("ByteProperty");
constexpr FName NAME_Int8Property("Int8Property");
constexpr FName NAME_Int16Property("Int16Property");
constexpr FName NAME_IntProperty("IntProperty");
constexpr FName NAME_Int64Property("Int64Property");
constexpr FName NAME_UInt16Property("UInt16Property");
constexpr FName NAME_UInt32Property("UInt32Property");
constexpr FName NAME_UInt64Property("UInt64Property");
constexpr FName NAME_FloatProperty("FloatProperty");
constexpr FName NAME_DoubleProperty("DoubleProperty");
constexpr FName NAME_NameProperty("NameProperty");
constexpr FName NAME_StrProperty("StrProperty");
constexpr FName NAME_TextProperty("TextProperty");
constexpr FName NAME_EnumProperty("EnumProperty");
constexpr FName NAME_ObjectProperty("ObjectProperty");
constexpr FName NAME_ClassProperty("ClassProperty");
constexpr FName NAME_WeakObjectProperty("WeakObjectProperty");
constexpr FName NAME_LazyObjectProperty("LazyObjectProperty");
constexpr FName NAME_SoftObjectProperty("SoftObjectProperty");
constexpr FName NAME_SoftClassProperty("SoftClassProperty");
constexpr FName NAME_AssetObjectProperty("AssetObjectProperty");
constexpr FName NAME_AssetClassProperty("AssetClassProperty");
constexpr FName NAME_InterfaceProperty("InterfaceProperty");
constexpr FName NAME_DelegateProperty("DelegateProperty");
constexpr FName NAME_MulticastDelegateProperty("MulticastDelegateProperty");
constexpr FName NAME_MulticastInlineDelegateProperty("MulticastInlineDelegateProperty");
constexpr FName NAME_MulticastSparseDelegateProperty("MulticastSparseDelegateProperty");
constexpr FName NAME_FieldPathProperty("FieldPathProperty");
constexpr FName NAME_StructProperty("StructProperty");
constexpr FName NAME_ArrayProperty("ArrayProperty");
constexpr FName NAME_SetProperty("SetProperty");
constexpr FName NAME_MapProperty("MapProperty");
constexpr FName NAME_OptionalProperty("OptionalProperty");

template <class TProperty>
TProperty* NewObjectReference(FFieldVariant Owner, FName Name) {
    auto* Property = new TProperty(Owner, Name);
    Property->PropertyClass = UObject::StaticClass();
    return Property;
}

FProperty* NewProperty(FName Name, const FPropertyTypeName& Type, FFieldVariant Owner);

template <class TContainer>
FProperty* NewContainer(FName Name, const FPropertyTypeName& Type, FFieldVariant Owner, FProperty* TContainer::* Inner) {
    auto Container = std::make_unique<TContainer>(Owner, Name);
    Container.get()->*Inner = NewProperty(Name, Type.GetParameter(0), Container.get());
    return Container.get()->*Inner != nullptr ? Container.release() : nullptr;
}

//! Before INNER_ARRAY_TAG_INFO and in a set or map, a tag does not name the struct of its elements.
FProperty* NewStruct(FName Name, FName StructName, FFieldVariant Owner) {
    if (StructName.isNone())
        StructName = NAME_StructProperty;
    UScriptStruct* Struct = FindNativeStruct(StructName);
    auto* Property = new FStructProperty(Owner, Name);
    Property->Struct = Struct != nullptr ? Struct : GetUnknownStruct(StructName);
    return Property;
}

FProperty* NewMap(FName Name, const FPropertyTypeName& Type, FFieldVariant Owner) {
    auto Map = std::make_unique<FMapProperty>(Owner, Name);
    Map->KeyProp = NewProperty(Name, Type.GetParameter(0), Map.get());
    Map->ValueProp = Map->KeyProp != nullptr ? NewProperty(Name, Type.GetParameter(1), Map.get()) : nullptr;
    return Map->ValueProp != nullptr ? Map.release() : nullptr;
}

//! A tagged enum value is a name, so the store keeps it as one when the enum is not loaded.
FProperty* NewProperty(FName Name, const FPropertyTypeName& Type, FFieldVariant Owner) {
    const FName Kind = Type.GetName();
    if (Kind == NAME_BoolProperty) {
        auto* Bool = new FBoolProperty(Owner, Name);
        Bool->SetBoolSize(1, true);
        return Bool;
    }
    if (Kind == NAME_ByteProperty)
        return Type.GetParameterName(0).isNone() ? static_cast<FProperty*>(new FByteProperty(Owner, Name)) : new FNameProperty(Owner, Name);
    if (Kind == NAME_EnumProperty || Kind == NAME_NameProperty)
        return new FNameProperty(Owner, Name);
    if (Kind == NAME_Int8Property)
        return new FInt8Property(Owner, Name);
    if (Kind == NAME_Int16Property)
        return new FInt16Property(Owner, Name);
    if (Kind == NAME_IntProperty)
        return new FIntProperty(Owner, Name);
    if (Kind == NAME_Int64Property)
        return new FInt64Property(Owner, Name);
    if (Kind == NAME_UInt16Property)
        return new FUInt16Property(Owner, Name);
    if (Kind == NAME_UInt32Property)
        return new FUInt32Property(Owner, Name);
    if (Kind == NAME_UInt64Property)
        return new FUInt64Property(Owner, Name);
    if (Kind == NAME_FloatProperty)
        return new FFloatProperty(Owner, Name);
    if (Kind == NAME_DoubleProperty)
        return new FDoubleProperty(Owner, Name);
    if (Kind == NAME_StrProperty)
        return new FStrProperty(Owner, Name);
    if (Kind == NAME_TextProperty)
        return new FTextProperty(Owner, Name);
    if (Kind == NAME_FieldPathProperty)
        return new FFieldPathProperty(Owner, Name);
    if (Kind == NAME_ObjectProperty || Kind == NAME_ClassProperty)
        return NewObjectReference<FObjectProperty>(Owner, Name);
    if (Kind == NAME_WeakObjectProperty)
        return NewObjectReference<FWeakObjectProperty>(Owner, Name);
    if (Kind == NAME_LazyObjectProperty)
        return NewObjectReference<FLazyObjectProperty>(Owner, Name);
    if (Kind == NAME_SoftObjectProperty || Kind == NAME_SoftClassProperty || Kind == NAME_AssetObjectProperty || Kind == NAME_AssetClassProperty)
        return NewObjectReference<FSoftObjectProperty>(Owner, Name);
    if (Kind == NAME_InterfaceProperty)
        return new FInterfaceProperty(Owner, Name);
    if (Kind == NAME_DelegateProperty)
        return new FDelegateProperty(Owner, Name);
    if (Kind == NAME_MulticastDelegateProperty || Kind == NAME_MulticastInlineDelegateProperty)
        return new FMulticastInlineDelegateProperty(Owner, Name);
    if (Kind == NAME_MulticastSparseDelegateProperty)
        return new FMulticastSparseDelegateProperty(Owner, Name);
    if (Kind == NAME_StructProperty)
        return NewStruct(Name, Type.GetParameterName(0), Owner);
    if (Kind == NAME_ArrayProperty)
        return NewContainer(Name, Type, Owner, &FArrayProperty::Inner);
    if (Kind == NAME_SetProperty)
        return NewContainer(Name, Type, Owner, &FSetProperty::ElementProp);
    if (Kind == NAME_OptionalProperty)
        return NewContainer(Name, Type, Owner, &FOptionalProperty::ValueProperty);
    if (Kind == NAME_MapProperty)
        return NewMap(Name, Type, Owner);
    return nullptr;
}

//! Before the complete type names, an array tag names only StructProperty; its data starts with the inner tag that names the struct.
FPropertyTypeName GetTypeName(FArchive& Ar, const FPropertyTag& Tag) {
    if (Tag.Type != NAME_ArrayProperty || Tag.TypeName.GetParameterName(0) != NAME_StructProperty || !Tag.TypeName.GetParameter(0).GetParameterName(0).isNone())
        return Tag.TypeName;
    const bool bInnerTag = FArrayProperty::HasInnerTag(Ar);
    if (!bInnerTag && Ar.GetGame() != EGame::GAME_DaysGone)
        return Tag.TypeName;
    const int64 Start = Ar.Tell();
    const int32 Count = Ar.ReadCount(0);
    FName StructName;
    if (bInnerTag) {
        FPropertyTag InnerTag;
        Ar << InnerTag;
        StructName = InnerTag.TypeName.GetParameterName(0);
    } else {
        const std::string_view Guess = guessArrayStructType(Ar.GetGame(), Tag.Name.getText(), (Tag.Size - 4) / std::max(Count, 1));
        if (!Guess.empty())
            StructName = FName(Guess);
    }
    Ar.Seek(Start);
    if (StructName.isNone())
        return Tag.TypeName;
    FPropertyTypeName TypeName;
    TypeName.AddNode(NAME_ArrayProperty, 1);
    TypeName.AddNode(NAME_StructProperty, 1);
    TypeName.AddNode(StructName, 0);
    return TypeName;
}

std::unique_ptr<FProperty> CreatePropertyFromTag(FArchive& Ar, const FPropertyTag& Tag) {
    const FPropertyTypeName TypeName = GetTypeName(Ar, Tag);
    if (Ar.IsError())
        return nullptr;
    std::unique_ptr<FProperty> Property(NewProperty(Tag.Name, TypeName, FFieldVariant()));
    if (Property == nullptr)
        return nullptr;
    const FStructProperty* Struct = CastField<FStructProperty>(Property.get());
    if (Struct != nullptr && Struct->Struct->HasAnyFlags(RF_Transient) && Tag.SerializeType == EPropertyTagSerializeType::BinaryOrNative)
        return nullptr;
    Property->Link();
    return Property;
}

FString DescribeTag(const FPropertyTag& Tag) {
    FString Text = FString(Tag.Name.toString()) + FString(" of type ") + FString(Tag.Type.toString());
    const FName Parameter = Tag.TypeName.GetParameterName(0);
    return Parameter.isNone() ? Text : Text + FString(" ") + FString(Parameter.toString());
}

}

FUnknownProperty::FUnknownProperty(const void* InContainer, int32 InArrayIndex, std::unique_ptr<FProperty> InProperty)
    : Container(InContainer), ArrayIndex(InArrayIndex), Property(std::move(InProperty)) {
    Value = RuntimeMalloc(static_cast<std::size_t>(std::max(Property->GetSize(), 1)), static_cast<std::size_t>(std::max(Property->GetMinAlignment(), 16)));
    std::memset(Value, 0, static_cast<std::size_t>(std::max(Property->GetSize(), 1)));
    Property->InitializeValue(Value);
}

FUnknownProperty::FUnknownProperty(FUnknownProperty&& Other) noexcept
    : Container(Other.Container), ArrayIndex(Other.ArrayIndex), Property(std::move(Other.Property)), Value(Other.Value) {
    Other.Value = nullptr;
}

FUnknownProperty& FUnknownProperty::operator=(FUnknownProperty&& Other) noexcept {
    if (this == &Other)
        return *this;
    Reset();
    Container = Other.Container;
    ArrayIndex = Other.ArrayIndex;
    Property = std::move(Other.Property);
    Value = Other.Value;
    Other.Value = nullptr;
    return *this;
}

FUnknownProperty::~FUnknownProperty() {
    Reset();
}

void FUnknownProperty::Reset() {
    if (Value == nullptr)
        return;
    Property->DestroyValue(Value);
    RuntimeFree(Value);
    Value = nullptr;
}

void FUnknownProperties::SortByContainer() {
    std::ranges::stable_sort(Properties, std::less<>(), &FUnknownProperty::GetContainer);
}

std::span<const FUnknownProperty> FUnknownProperties::FindByContainer(const void* Container) const {
    auto [First, Last] = std::ranges::equal_range(Properties, Container, std::less<>(), &FUnknownProperty::GetContainer);
    return {First, Last};
}

const FUnknownProperty* FUnknownProperties::Find(const void* Container, FName Name) const {
    const auto Found = std::ranges::find_if(Properties, [&](const FUnknownProperty& Property) {
        return Property.GetContainer() == Container && Property.GetArrayIndex() == 0 && Property.GetProperty()->GetFName() == Name;
    });
    return Found == Properties.end() ? nullptr : &*Found;
}

FLoadedPropertyValue FindLoadedPropertyValue(FArchive& Ar, const UStruct* Struct, const void* Container, FName Name) {
    if (const FProperty* Property = Struct->FindPropertyByName(Name))
        return {Property, Property->ContainerPtrToValuePtr<void>(Container)};
    const FUnknownProperties* Store = Ar.GetUnknownProperties();
    const FUnknownProperty* Unknown = Store == nullptr ? nullptr : Store->Find(Container, Name);
    if (Unknown == nullptr)
        return {};
    return {Unknown->GetProperty(), Unknown->GetValue()};
}

bool GetLoadedBoolPropertyValue(FArchive& Ar, const UObject* Object, FName Name) {
    const FLoadedPropertyValue Found = FindLoadedPropertyValue(Ar, Object->GetClass(), Object, Name);
    const FBoolProperty* Bool = CastField<FBoolProperty>(Found.Property);
    return Bool != nullptr && Bool->GetPropertyValue(Found.Value);
}

uint8 GetLoadedBytePropertyValue(FArchive& Ar, const UObject* Object, FName Name) {
    const FLoadedPropertyValue Found = FindLoadedPropertyValue(Ar, Object->GetClass(), Object, Name);
    const FByteProperty* Byte = CastField<FByteProperty>(Found.Property);
    return Byte == nullptr ? 0 : Byte->GetPropertyValue(Found.Value);
}

UObject* GetLoadedObjectPropertyValue(FArchive& Ar, const UObject* Object, FName Name) {
    const FLoadedPropertyValue Found = FindLoadedPropertyValue(Ar, Object->GetClass(), Object, Name);
    const FObjectPropertyBase* Reference = CastField<FObjectPropertyBase>(Found.Property);
    return Reference == nullptr ? nullptr : Reference->GetObjectPropertyValue(Found.Value);
}

void KeepUnknownProperty(FArchive& Ar, const FPropertyTag& Tag, void* Container) {
    FUnknownProperties* Store = Ar.GetUnknownProperties();
    FLinkerLoad* Linker = Ar.GetLinker();
    if (Store == nullptr || Linker == nullptr)
        return;
    std::unique_ptr<FProperty> Property = CreatePropertyFromTag(Ar, Tag);
    if (Property == nullptr) {
        ReportLoadProblem("load.unknown_type", DescribeTag(Tag));
        return;
    }
    FUnknownProperty Unknown(Container, Tag.ArrayIndex, std::move(Property));
    if (const FBoolProperty* Bool = CastField<FBoolProperty>(Unknown.GetProperty())) {
        Bool->SetPropertyValue(Unknown.GetValue(), Tag.BoolVal != 0);
        Store->Add(std::move(Unknown));
        return;
    }
    const int64 CookedPosition = Ar.GetCookedPosition();
    const int64 ChunkPosition = Ar.GetExportChunkPosition();
    const std::span<const std::byte> Bytes = Ar.ReadView(Tag.Size);
    if (Ar.IsError())
        return;
    const std::size_t Mark = Store->Num();
    FExportArchive ValueAr(*Linker, Bytes, CookedPosition, ChunkPosition, Store);
    Unknown.GetProperty()->SerializeItem(ValueAr, Unknown.GetValue());
    if (!ValueAr.IsError() && ValueAr.AtEnd()) {
        Store->Add(std::move(Unknown));
        return;
    }
    Store->Truncate(Mark);
    const FString Reason = ValueAr.IsError() ? FString(getErrorName(ValueAr.GetError().code)) + FString(" at ") + FString(ValueAr.GetError().where)
                                             : FString("unread bytes");
    ReportLoadProblem("load.unknown_value", DescribeTag(Tag) + FString(" (") + Reason + FString(")"));
}

FProperty* NewPropertyFromTypeName(FName Name, const FPropertyTypeName& Type, FFieldVariant Owner) {
    return NewProperty(Name, Type, Owner);
}

FPropertyTypeName GetPropertyTagTypeName(FArchive& Ar, const FPropertyTag& Tag) {
    return GetTypeName(Ar, Tag);
}

}
