#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "ue5parse/compress/Decompressor.h"
#include "ue5parse/core/Arena.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/io/FileSource.h"

namespace ue5parse {

// EPropertyType of the .usmap format. The values are the file's; they are not UE's.
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

// One property type: a node of a type tree. Containers point to their element types. The nodes of the mappings live
// as long as their Mappings; the nodes of a Blueprint class live as long as the FileProvider that loaded it.
struct PropertyType {
    EPropertyType type = EPropertyType::Unknown;
    const PropertyType* inner = nullptr;   // array, set and optional element; map key; the underlying type of an enum
    const PropertyType* value = nullptr;   // map value
    FName structName;                      // StructProperty
    // StructProperty: the schema, when it is known where the type was made (a Blueprint's UserDefinedStruct).
    // Null for the mappings: their structs are looked up by name, because the mappings do not know which are native.
    const StructSchema* structSchema = nullptr;
    FName enumName;                        // EnumProperty
    // EnumProperty: the entries, when they are known where the type was made (a Blueprint's UserDefinedEnum).
    const EnumSchema* enumSchema = nullptr;
};

// The EPropertyType of a property class name ("ObjectProperty", "ObjectPtrProperty"): Unknown for another name.
EPropertyType getPropertyTypeByName(std::string_view name);
const char* getPropertyTypeName(EPropertyType type);

// One serializable property of a struct. A static array (ArrayDim > 1) is one entry for each element, as the
// unversioned format counts them.
struct PropertyInfo {
    FName name;
    const PropertyType* type = nullptr;
    std::uint16_t index = 0;       // the schema index in its struct, with the array elements counted
    std::uint8_t arrayIndex = 0;   // which element of a static array
    std::uint8_t arrayDim = 1;
};

// One class or struct: its own properties, and its super struct. From the mappings, or made from a Blueprint class.
struct StructSchema {
    FName name;
    FName superName;
    const StructSchema* super = nullptr;   // null when there is no super or it is not known
    // The size of this struct's own index space. The indices below it belong to this struct, the ones above to the
    // super chain. Some indices have no serializable property.
    std::uint16_t propertyCount = 0;
    std::span<const PropertyInfo> properties;       // the own properties, sorted by index
    std::span<const PropertyInfo* const> slots;     // propertyCount entries: own index -> property, or null
    // Mappings only: this struct's name, then its supers' names to the root; a super that the mappings do not have
    // ends it. Made at parse, so that a lookup needs no lock.
    std::span<const FName> nameChain;
};

// The property of an index of a struct's full index space (own properties first, then the super chain), or null when
// no serializable property has it. O(depth of the chain).
const PropertyInfo* findProperty(const StructSchema& schema, std::uint32_t index);

struct EnumSchema {
    FName name;
    std::span<const FName> names;
    std::span<const std::uint64_t> values;

    // The name of a value, or null when the value is not an entry of the enum.
    const FName* findName(std::uint64_t value) const;
};

// A parsed .usmap file: the property schemas of every class and struct of a game, for unversioned properties. Owns
// all its names and nodes. Immutable after construction, so it is thread-safe; a move keeps every pointer valid.
class Mappings {
public:
    Mappings();
    Mappings(Mappings&&) noexcept;
    Mappings& operator=(Mappings&&) noexcept;
    ~Mappings();

    std::size_t getStructCount() const { return structs.size(); }
    const StructSchema& getStruct(std::size_t index) const { return structs[index]; }
    // O(1). Ignores case, as UE names do.
    const StructSchema* findStruct(FName name) const;

    std::size_t getEnumCount() const { return enums.size(); }
    const EnumSchema& getEnum(std::size_t index) const { return enums[index]; }
    const EnumSchema* findEnum(FName name) const;

private:
    friend Result<Mappings> parseMappings(std::span<const std::byte> file, const Codecs& codecs);
    struct Index;

    std::unique_ptr<Arena> storage;   // the name texts
    std::vector<StructSchema> structs;
    std::vector<PropertyInfo> properties;
    std::vector<const PropertyInfo*> slots;
    std::vector<PropertyType> types;
    std::vector<EnumSchema> enums;
    std::vector<FName> enumNames;
    std::vector<std::uint64_t> enumValues;
    std::unique_ptr<Index> index;
};

// Parses a whole .usmap file (versions Initial to EngineVersioning; uncompressed, Oodle, Brotli or Zstd).
Result<Mappings> parseMappings(std::span<const std::byte> file, const Codecs& codecs);
Result<Mappings> readMappings(const FileSource& file, const Codecs& codecs);

}
