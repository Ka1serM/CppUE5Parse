#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Core/Arena.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/IO/FileSource.h"

namespace ue5parse {

//! The values are the .usmap file's, not UE's.
enum class EPropertyType : std::uint8_t {
    ByteProperty,
    BoolProperty,
    IntProperty,
    FloatProperty,
    ObjectProperty,
    NameProperty,
    DelegateProperty,
    DoubleProperty,
    ArrayProperty,
    StructProperty,
    StrProperty,
    TextProperty,
    InterfaceProperty,
    MulticastDelegateProperty,
    WeakObjectProperty,
    LazyObjectProperty,
    AssetObjectProperty,
    SoftObjectProperty,
    UInt64Property,
    UInt32Property,
    UInt16Property,
    Int64Property,
    Int16Property,
    Int8Property,
    MapProperty,
    SetProperty,
    EnumProperty,
    FieldPathProperty,
    OptionalProperty,
    Utf8StrProperty,
    AnsiStrProperty,
    ClassProperty,
    MulticastInlineDelegateProperty,
    SoftClassProperty,
    VerseStringProperty,
    VerseDynamicProperty,
    VerseFunctionProperty,
    Unknown = 0xFF,
};

struct StructSchema;
struct EnumSchema;

struct PropertyType {
    EPropertyType type = EPropertyType::Unknown;
    const PropertyType* inner = nullptr;
    const PropertyType* value = nullptr;
    FName structName;
    const StructSchema* structSchema = nullptr;
    FName enumName;
    const EnumSchema* enumSchema = nullptr;
};

EPropertyType getPropertyTypeByName(std::string_view name);
const char* getPropertyTypeName(EPropertyType type);

struct PropertyInfo {
    FName name;
    const PropertyType* type = nullptr;
    std::uint16_t index = 0;
    std::uint8_t arrayIndex = 0;
    std::uint8_t arrayDim = 1;
};

struct StructSchema {
    FName name;
    FName superName;
    const StructSchema* super = nullptr;
    std::uint16_t propertyCount = 0;
    std::span<const PropertyInfo> properties;
    std::span<const PropertyInfo* const> slots;
    std::span<const FName> nameChain;
};

const PropertyInfo* findProperty(const StructSchema& schema, std::uint32_t index);

struct EnumSchema {
    FName name;
    std::span<const FName> names;
    std::span<const std::uint64_t> values;
};

class Mappings {
public:
    Mappings();
    Mappings(Mappings&&) noexcept;
    Mappings& operator=(Mappings&&) noexcept;
    ~Mappings();

    std::size_t getStructCount() const { return structs.size(); }
    const StructSchema& getStruct(std::size_t index) const { return structs[index]; }
    const StructSchema* findStruct(FName name) const;

    std::size_t getEnumCount() const { return enums.size(); }
    const EnumSchema* findEnum(FName name) const;

private:
    friend Result<Mappings> parseMappings(std::span<const std::byte> file, const Codecs& codecs);
    struct Index;

    std::unique_ptr<Arena> storage;
    std::vector<StructSchema> structs;
    std::vector<PropertyInfo> properties;
    std::vector<const PropertyInfo*> slots;
    std::vector<PropertyType> types;
    std::vector<EnumSchema> enums;
    std::vector<FName> enumNames;
    std::vector<std::uint64_t> enumValues;
    std::unique_ptr<Index> index;
};

Result<Mappings> parseMappings(std::span<const std::byte> file, const Codecs& codecs);
Result<Mappings> readMappings(const FileSource& file, const Codecs& codecs);

}
