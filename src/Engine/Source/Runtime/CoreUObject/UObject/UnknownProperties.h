#pragma once

#include <memory>
#include <span>
#include <vector>

#include "Engine/Source/Runtime/CoreUObject/UObject/Field.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

class FArchive;
class FProperty;
class UObject;
class UStruct;

class FUnknownProperty {
public:
    FUnknownProperty(const void* InContainer, int32 InArrayIndex, std::unique_ptr<FProperty> InProperty);
    FUnknownProperty(FUnknownProperty&& Other) noexcept;
    FUnknownProperty& operator=(FUnknownProperty&& Other) noexcept;
    ~FUnknownProperty();

    const void* GetContainer() const { return Container; }
    int32 GetArrayIndex() const { return ArrayIndex; }
    const FProperty* GetProperty() const { return Property.get(); }
    void* GetValue() const { return Value; }

private:
    void Reset();

    const void* Container;
    int32 ArrayIndex;
    std::unique_ptr<FProperty> Property;
    void* Value = nullptr;
};

class FUnknownProperties {
public:
    std::size_t Num() const { return Properties.size(); }
    void Add(FUnknownProperty&& Property) { Properties.push_back(std::move(Property)); }
    void Truncate(std::size_t Count) { Properties.erase(Properties.begin() + static_cast<std::ptrdiff_t>(Count), Properties.end()); }
    void SortByContainer();
    std::span<const FUnknownProperty> FindByContainer(const void* Container) const;
    const FUnknownProperty* Find(const void* Container, FName Name) const;

private:
    std::vector<FUnknownProperty> Properties;
};

void KeepUnknownProperty(FArchive& Ar, const FPropertyTag& Tag, void* Container);

//! Creates the property of a tag type, or nullptr for a type that has no property; an enum or a missing struct keeps its value as a name or as unknown tags.
FProperty* NewPropertyFromTypeName(FName Name, const FPropertyTypeName& Type, FFieldVariant Owner);
//! The type of a tag at the start of its value, with the struct of an array of structs that an older tag does not name.
FPropertyTypeName GetPropertyTagTypeName(FArchive& Ar, const FPropertyTag& Tag);

struct FLoadedPropertyValue {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
};

//! Finds a property value that a game reads during its binary serialization, in the struct or in the unknown tags of the archive.
FLoadedPropertyValue FindLoadedPropertyValue(FArchive& Ar, const UStruct* Struct, const void* Container, FName Name);
bool GetLoadedBoolPropertyValue(FArchive& Ar, const UObject* Object, FName Name);
uint8 GetLoadedBytePropertyValue(FArchive& Ar, const UObject* Object, FName Name);
UObject* GetLoadedObjectPropertyValue(FArchive& Ar, const UObject* Object, FName Name);

}
