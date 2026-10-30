#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "ue5parse/core/ByteReader.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/core/Versions.h"
#include "ue5parse/schema/Mappings.h"

namespace ue5parse {

// Where a value is serialized, which changes how some types are written (UE's unversioned serializer writes numbers
// and enums at the top level as plain integers, but containers call SerializeItem, which writes enums as names).
enum class ReadContext : std::uint8_t { Normal, Array, Map, Optional };

// What reading property data needs from its package and game.
struct PropertyContext {
    const Mappings* mappings = nullptr;   // for the structs that a type names but does not point to
    std::span<const FName> names;   // the package's name map, which serialized FNames index
    EngineVersion engine = latestEngineVersion;
};

// One serialized property of an object or struct: its schema entry, and where its value is. Values are decoded only
// when they are read (design §8).
struct FPropertyTag {
    FName name;
    const PropertyType* type = nullptr;
    std::uint32_t offset = 0;    // of the value in the data the properties were read from
    std::uint32_t size = 0;
    std::uint8_t arrayIndex = 0;
    bool zero = false;           // serialized as zero: the value has no bytes
};

// Receives the tags of readUnversionedProperties one at a time.
class TagSink {
public:
    virtual ~TagSink() = default;
    // False stops the read after this tag.
    virtual bool add(const FPropertyTag& tag) = 0;
};

// Reads the unversioned properties of a struct or object (FUnversionedHeader, then the values) from the reader's
// position, and gives one tag for each to the sink. The offsets count from the start of the reader's data.
Result<void> readUnversionedProperties(ByteReader& reader, const PropertyContext& context, const StructSchema& schema,
                                       TagSink& sink);
// The same, appending the tags to a vector.
Result<void> readUnversionedProperties(ByteReader& reader, const PropertyContext& context, const StructSchema& schema,
                                       std::vector<FPropertyTag>& out);
// The tag of one property, without keeping the others: reads up to it. None when it is not serialized.
Result<std::optional<FPropertyTag>> findUnversionedProperty(ByteReader& reader, const PropertyContext& context,
                                                            const StructSchema& schema, FName name, std::uint32_t arrayIndex);

// Moves the reader past one value of a type.
Result<void> skipPropertyValue(ByteReader& reader, const PropertyContext& context, const PropertyType& type, ReadContext where);

// Whether a struct serializes natively (UE's WithSerializer: FVector, FGuid, FSoftObjectPath, ...), so that its value
// is only bytes, not unversioned properties.
bool isNativeStruct(FName structName);

// An FName in serialized data: index into the package's name map, and the number. Fails on a bad index.
FName readName(ByteReader& reader, const PropertyContext& context);

}
