#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/Schema/Mappings.h"

namespace ue5parse {

enum class ReadContext : std::uint8_t { Normal, Array, Map, Optional };

struct PropertyContext {
    const Mappings* mappings = nullptr;
    std::span<const FName> names;
    EGame game = EGame::GAME_UE5_LATEST;
};

struct FPropertyTag {
    FName name;
    const PropertyType* type = nullptr;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
    std::uint8_t arrayIndex = 0;
    bool zero = false;
};

class TagSink {
public:
    virtual ~TagSink() = default;
    virtual bool add(const FPropertyTag& tag) = 0;
};

Result<void> readUnversionedProperties(ByteReader& reader, const PropertyContext& context, const StructSchema& schema,
                                       TagSink& sink);
Result<void> readUnversionedProperties(ByteReader& reader, const PropertyContext& context, const StructSchema& schema,
                                       std::vector<FPropertyTag>& out);
Result<std::optional<FPropertyTag>> findUnversionedProperty(ByteReader& reader, const PropertyContext& context,
                                                            const StructSchema& schema, FName name, std::uint32_t arrayIndex);

Result<void> skipPropertyValue(ByteReader& reader, const PropertyContext& context, const PropertyType& type, ReadContext where);

bool isNativeStruct(FName structName);

FName readName(ByteReader& reader, const PropertyContext& context);

}
