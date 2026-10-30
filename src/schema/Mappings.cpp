#include <ankerl/unordered_dense.h>

#include <algorithm>
#include <cstring>

#include "ue5parse/core/ByteReader.h"
#include "ue5parse/schema/Mappings.h"

namespace ue5parse {

namespace {

constexpr std::uint16_t usmapMagic = 0x30C4;

// EUsmapVersion.
enum class EUsmapVersion : std::uint8_t {
    Initial,
    PackageVersioning,
    LongFName,
    LargeEnums,
    ExplicitEnumValues,
    EngineVersioning,
};
constexpr EUsmapVersion latestVersion = EUsmapVersion::EngineVersioning;

enum class EUsmapCompressionMethod : std::uint8_t { None, Oodle, Brotli, ZStandard };

// A type tree deeper than this is corrupt: UE has no nesting of containers that deep.
constexpr int maxTypeDepth = 32;

constexpr std::uint32_t none = ~0u;

// The nodes while the file is read: indices, because the vectors still grow.
struct TempType {
    EPropertyType type = EPropertyType::Unknown;
    std::uint32_t inner = none;
    std::uint32_t value = none;
    FName structName;
    FName enumName;
};

struct TempProperty {
    FName name;
    std::uint32_t type;
    std::uint16_t index;
    std::uint8_t arrayIndex;
    std::uint8_t arrayDim;
};

struct TempStruct {
    FName name;
    FName superName;
    std::uint16_t propertyCount;
    std::uint32_t firstProperty;
    std::uint32_t propertyEnd;
};

struct NameHash {
    using is_avalanching = void;
    std::uint64_t operator()(const FName& name) const noexcept { return name.getHash() ^ name.getNumber(); }
};

class UsmapReader {
public:
    UsmapReader(std::span<const std::byte> data, EUsmapVersion version, std::span<const FName> names)
        : reader(data), version(version), names(names) {}

    // A name of the name table, or None for index -1.
    FName readName() {
        std::int32_t index = reader.read<std::int32_t>();
        if (index == -1) return FName();
        if (index < 0 || static_cast<std::size_t>(index) >= names.size()) {
            reader.fail(ErrorCode::InvalidData, "parseMappings.name");
            return FName();
        }
        return names[static_cast<std::size_t>(index)];
    }

    std::uint32_t readType(std::vector<TempType>& types, int depth) {
        if (depth > maxTypeDepth) {
            reader.fail(ErrorCode::InvalidData, "parseMappings.typeDepth");
            return 0;
        }
        TempType type;
        type.type = static_cast<EPropertyType>(reader.read<std::uint8_t>());
        switch (type.type) {
        case EPropertyType::EnumProperty:
            type.inner = readType(types, depth + 1);
            type.enumName = readName();
            break;
        case EPropertyType::StructProperty:
            type.structName = readName();
            break;
        case EPropertyType::SetProperty:
        case EPropertyType::ArrayProperty:
        case EPropertyType::OptionalProperty:
            type.inner = readType(types, depth + 1);
            break;
        case EPropertyType::MapProperty:
            type.inner = readType(types, depth + 1);
            type.value = readType(types, depth + 1);
            break;
        default:
            break;
        }
        types.push_back(type);
        return static_cast<std::uint32_t>(types.size() - 1);
    }

    ByteReader reader;
    EUsmapVersion version;
    std::span<const FName> names;
};

// FName from usmap text. The usmap names are whole names: "Mesh_3" is a property name, not "Mesh" number 4, so the
// text is not split.
FName makeName(std::string_view text) { return FName(text, hashName(text), 0); }

}

struct Mappings::Index {
    ankerl::unordered_dense::map<FName, std::uint32_t, NameHash> structs;
    ankerl::unordered_dense::map<FName, std::uint32_t, NameHash> enums;
};

Mappings::Mappings() : storage(std::make_unique<Arena>()), index(std::make_unique<Index>()) {}
Mappings::Mappings(Mappings&&) noexcept = default;
Mappings& Mappings::operator=(Mappings&&) noexcept = default;
Mappings::~Mappings() = default;

const StructSchema* Mappings::findStruct(FName name) const {
    auto found = index->structs.find(name);
    return found != index->structs.end() ? &structs[found->second] : nullptr;
}

const EnumSchema* Mappings::findEnum(FName name) const {
    auto found = index->enums.find(name);
    return found != index->enums.end() ? &enums[found->second] : nullptr;
}

const PropertyInfo* findProperty(const StructSchema& schema, std::uint32_t propertyIndex) {
    const StructSchema* current = &schema;
    // A cycle in corrupt data ends at this depth; UE class chains are far shorter.
    for (int depth = 0; depth < 256 && current != nullptr; depth++) {
        if (propertyIndex < current->propertyCount) return current->slots[propertyIndex];
        propertyIndex -= current->propertyCount;
        current = current->super;
    }
    return nullptr;
}

const FName* EnumSchema::findName(std::uint64_t value) const {
    // The last entry of a value, as CUE4Parse's value -> name table keeps it: UE5's BLEND_TranslucentGreyTransmittance
    // has the value of BLEND_Translucent and comes after it.
    for (std::size_t i = values.size(); i-- > 0;)
        if (values[i] == value) return &names[i];
    return nullptr;
}

Result<Mappings> parseMappings(std::span<const std::byte> file, const Codecs& codecs) {
    ByteReader header{file};
    std::uint16_t magic = header.read<std::uint16_t>();
    EUsmapVersion version = static_cast<EUsmapVersion>(header.read<std::uint8_t>());
    if (header.hasFailed()) return std::unexpected(header.getError());
    if (magic != usmapMagic) return fail(ErrorCode::BadMagic, "parseMappings.magic");
    if (version > latestVersion) return fail(ErrorCode::UnsupportedVersion, "parseMappings.version");

    // The versioning block is only metadata: the profile, not the mappings, gives the engine version.
    bool hasVersioning = version >= EUsmapVersion::PackageVersioning && header.read<std::int32_t>() != 0;
    if (hasVersioning) {
        if (version >= EUsmapVersion::EngineVersioning) {
            header.skip(2 * 3 + 4);   // FEngineVersionBase: major, minor, patch, changelist
            bool wide = false;
            header.readStringBytes(wide);   // the branch
        }
        header.skip(4 + 4);   // FPackageFileVersion
        std::uint32_t customVersions = header.readCount(20);
        header.skip(std::size_t(customVersions) * 20);
        header.skip(4);   // NetCL
    }
    EUsmapCompressionMethod method = static_cast<EUsmapCompressionMethod>(header.read<std::uint8_t>());
    std::uint32_t compressedSize = header.read<std::uint32_t>();
    std::uint32_t size = header.read<std::uint32_t>();
    std::span<const std::byte> compressed = header.readBytes(compressedSize);
    if (header.hasFailed()) return std::unexpected(header.getError());

    std::vector<std::byte> data;
    if (method == EUsmapCompressionMethod::None) {
        if (compressedSize != size) return fail(ErrorCode::InvalidData, "parseMappings.size");
        data.assign(compressed.begin(), compressed.end());
    } else {
        const char* codecName = method == EUsmapCompressionMethod::Oodle    ? "Oodle"
                                : method == EUsmapCompressionMethod::Brotli ? "Brotli"
                                : method == EUsmapCompressionMethod::ZStandard ? "Zstd"
                                                                             : nullptr;
        const Decompressor* decompressor = codecName != nullptr ? codecs.find(FName(codecName)) : nullptr;
        if (decompressor == nullptr) return fail(ErrorCode::UnknownCompression, "parseMappings.compression");
        // The codec may read and write past the ends.
        std::vector<std::byte> input(compressed.size() + decompressor->getInputSlack());
        std::memcpy(input.data(), compressed.data(), compressed.size());
        data.resize(size + decompressor->getOutputSlack());
        Result<void> done = decompressor->decompress({input.data(), compressed.size()}, {data.data(), size});
        if (!done) return std::unexpected(done.error());
        data.resize(size);
    }

    Mappings mappings;
    Arena& storage = *mappings.storage;

    // The name table.
    ByteReader names{data};
    std::uint32_t nameCount = names.readCount(1);
    std::vector<FName> nameTable;
    nameTable.reserve(nameCount);
    for (std::uint32_t i = 0; i < nameCount && !names.hasFailed(); i++) {
        std::size_t length = version >= EUsmapVersion::LongFName ? names.read<std::uint16_t>() : names.read<std::uint8_t>();
        std::span<const std::byte> bytes = names.readBytes(length);
        nameTable.push_back(makeName(storage.copy({reinterpret_cast<const char*>(bytes.data()), bytes.size()})));
    }
    if (names.hasFailed()) return std::unexpected(names.getError());

    UsmapReader usmap{std::span(data).subspan(names.getPosition()), version, nameTable};
    ByteReader& reader = usmap.reader;

    // Enums. A second enum with the same name is ignored, as in CUE4Parse.
    struct TempEnum {
        FName name;
        std::uint32_t first;
        std::uint32_t count;
    };
    std::vector<TempEnum> tempEnums;
    std::uint32_t enumCount = reader.read<std::uint32_t>();
    for (std::uint32_t i = 0; i < enumCount && !reader.hasFailed(); i++) {
        TempEnum entry;
        entry.name = usmap.readName();
        entry.count = version >= EUsmapVersion::LargeEnums ? reader.read<std::uint16_t>() : reader.read<std::uint8_t>();
        entry.first = static_cast<std::uint32_t>(mappings.enumNames.size());
        for (std::uint32_t j = 0; j < entry.count && !reader.hasFailed(); j++) {
            std::uint64_t value = j;
            if (version >= EUsmapVersion::ExplicitEnumValues) value = reader.read<std::uint64_t>();
            mappings.enumValues.push_back(value);
            mappings.enumNames.push_back(usmap.readName());
        }
        auto [found, inserted] = mappings.index->enums.try_emplace(entry.name, static_cast<std::uint32_t>(tempEnums.size()));
        if (inserted) tempEnums.push_back(entry);
    }

    // Structs. A later struct with the same name replaces the earlier one, as in CUE4Parse.
    std::vector<TempType> tempTypes;
    std::vector<TempProperty> tempProperties;
    std::vector<TempStruct> tempStructs;
    std::uint32_t structCount = reader.read<std::uint32_t>();
    if (!reader.hasFailed() && structCount > reader.getRemaining() / 12) return fail(ErrorCode::ReadPastEnd, "parseMappings.structs");
    tempStructs.reserve(structCount);
    for (std::uint32_t i = 0; i < structCount && !reader.hasFailed(); i++) {
        TempStruct entry;
        entry.name = usmap.readName();
        entry.superName = usmap.readName();
        entry.propertyCount = reader.read<std::uint16_t>();
        std::uint16_t serializable = reader.read<std::uint16_t>();
        entry.firstProperty = static_cast<std::uint32_t>(tempProperties.size());
        for (std::uint16_t j = 0; j < serializable && !reader.hasFailed(); j++) {
            std::uint16_t propertyIndex = reader.read<std::uint16_t>();
            std::uint8_t arrayDim = reader.read<std::uint8_t>();
            FName name = usmap.readName();
            std::uint32_t type = usmap.readType(tempTypes, 0);
            for (std::uint8_t k = 0; k < arrayDim; k++)
                tempProperties.push_back(TempProperty{name, type, static_cast<std::uint16_t>(propertyIndex + k), k, arrayDim});
        }
        entry.propertyEnd = static_cast<std::uint32_t>(tempProperties.size());
        auto own = std::span(tempProperties).subspan(entry.firstProperty, entry.propertyEnd - entry.firstProperty);
        std::sort(own.begin(), own.end(), [](const TempProperty& a, const TempProperty& b) { return a.index < b.index; });
        for (const TempProperty& property : own)
            if (property.index >= entry.propertyCount) return fail(ErrorCode::InvalidData, "parseMappings.propertyIndex");

        auto [found, inserted] = mappings.index->structs.try_emplace(entry.name, static_cast<std::uint32_t>(tempStructs.size()));
        if (inserted) tempStructs.push_back(entry);
        else tempStructs[found->second] = entry;
    }
    if (reader.hasFailed()) return std::unexpected(reader.getError());

    // The final nodes. The vectors get their sizes first, so the pointers between them stay valid.
    mappings.types.resize(tempTypes.size());
    for (std::size_t i = 0; i < tempTypes.size(); i++) {
        const TempType& from = tempTypes[i];
        PropertyType& to = mappings.types[i];
        to.type = from.type;
        to.inner = from.inner != none ? &mappings.types[from.inner] : nullptr;
        to.value = from.value != none ? &mappings.types[from.value] : nullptr;
        to.structName = from.structName;
        to.enumName = from.enumName;
    }
    mappings.properties.resize(tempProperties.size());
    for (std::size_t i = 0; i < tempProperties.size(); i++) {
        const TempProperty& from = tempProperties[i];
        mappings.properties[i] = PropertyInfo{from.name, &mappings.types[from.type], from.index, from.arrayIndex, from.arrayDim};
    }
    std::size_t slotCount = 0;
    for (const TempStruct& entry : tempStructs) slotCount += entry.propertyCount;
    mappings.slots.assign(slotCount, nullptr);
    mappings.structs.resize(tempStructs.size());
    std::size_t nextSlot = 0;
    for (std::size_t i = 0; i < tempStructs.size(); i++) {
        const TempStruct& from = tempStructs[i];
        StructSchema& to = mappings.structs[i];
        to.name = from.name;
        to.superName = from.superName;
        to.propertyCount = from.propertyCount;
        to.properties = std::span(mappings.properties).subspan(from.firstProperty, from.propertyEnd - from.firstProperty);
        std::span<const PropertyInfo*> slots = std::span(mappings.slots).subspan(nextSlot, from.propertyCount);
        for (const PropertyInfo& property : to.properties) slots[property.index] = &property;
        to.slots = slots;
        nextSlot += from.propertyCount;
    }
    for (StructSchema& schema : mappings.structs) {
        if (schema.superName.isNone()) continue;
        auto found = mappings.index->structs.find(schema.superName);
        if (found != mappings.index->structs.end()) schema.super = &mappings.structs[found->second];
    }
    std::vector<FName> chain;
    for (StructSchema& schema : mappings.structs) {
        chain.clear();
        // A cycle in corrupt mappings ends at this depth; UE class chains are far shorter.
        const StructSchema* current = &schema;
        for (int depth = 0; depth < 64 && current != nullptr; depth++) {
            chain.push_back(current->name);
            if (current->superName.isNone()) break;
            if (current->super == nullptr) chain.push_back(current->superName);
            current = current->super;
        }
        std::span<FName> stored = mappings.storage->allocate<FName>(chain.size());
        std::copy(chain.begin(), chain.end(), stored.begin());
        schema.nameChain = stored;
    }
    mappings.enums.resize(tempEnums.size());
    for (std::size_t i = 0; i < tempEnums.size(); i++) {
        mappings.enums[i].name = tempEnums[i].name;
        mappings.enums[i].names = std::span(mappings.enumNames).subspan(tempEnums[i].first, tempEnums[i].count);
        mappings.enums[i].values = std::span(mappings.enumValues).subspan(tempEnums[i].first, tempEnums[i].count);
    }
    return mappings;
}

Result<Mappings> readMappings(const FileSource& file, const Codecs& codecs) {
    std::vector<std::byte> bytes(file.getSize());
    Result<void> read = file.read(0, bytes);
    if (!read) return std::unexpected(read.error());
    return parseMappings(bytes, codecs);
}

namespace {

constexpr const char* propertyTypeNames[] = {
    "ByteProperty", "BoolProperty", "IntProperty", "FloatProperty", "ObjectProperty", "NameProperty",
    "DelegateProperty", "DoubleProperty", "ArrayProperty", "StructProperty", "StrProperty", "TextProperty",
    "InterfaceProperty", "MulticastDelegateProperty", "WeakObjectProperty", "LazyObjectProperty",
    "AssetObjectProperty", "SoftObjectProperty", "UInt64Property", "UInt32Property", "UInt16Property",
    "Int64Property", "Int16Property", "Int8Property", "MapProperty", "SetProperty", "EnumProperty",
    "FieldPathProperty", "OptionalProperty", "Utf8StrProperty", "AnsiStrProperty", "ClassProperty",
    "MulticastInlineDelegateProperty", "SoftClassProperty", "VerseStringProperty", "VerseDynamicProperty",
    "VerseFunctionProperty",
};

}

EPropertyType getPropertyTypeByName(std::string_view name) {
    if (name == "ObjectPtrProperty") return EPropertyType::ObjectProperty;
    if (name == "ClassPtrProperty") return EPropertyType::ClassProperty;
    if (name == "MulticastSparseDelegateProperty") return EPropertyType::MulticastDelegateProperty;
    for (std::size_t i = 0; i < sizeof(propertyTypeNames) / sizeof(propertyTypeNames[0]); i++)
        if (name == propertyTypeNames[i]) return static_cast<EPropertyType>(i);
    return EPropertyType::Unknown;
}

const char* getPropertyTypeName(EPropertyType type) {
    std::size_t index = static_cast<std::size_t>(type);
    if (index < sizeof(propertyTypeNames) / sizeof(propertyTypeNames[0])) return propertyTypeNames[index];
    return "Unknown";
}

}
