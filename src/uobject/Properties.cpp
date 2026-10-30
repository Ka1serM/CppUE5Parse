#include <ankerl/unordered_dense.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

#include "ue5parse/uobject/Properties.h"

namespace ue5parse {

namespace {

// A nesting of structs and containers deeper than this is corrupt data.
constexpr int maxDepth = 64;

// FUnversionedHeader fragment: 7 bits skip count, 1 bit "has zeroes", 1 bit "is last", 7 bits value count.
struct Fragment {
    std::uint8_t skipNum;
    bool hasAnyZeroes;
    bool isLast;
    std::uint8_t valueNum;
};

Fragment unpackFragment(std::uint16_t packed) {
    return Fragment{static_cast<std::uint8_t>(packed & 0x7F), (packed & 0x80) != 0, (packed & 0x100) != 0,
                    static_cast<std::uint8_t>(packed >> 9)};
}

// The structs that serialize natively (UE's WithSerializer) instead of as unversioned properties: CUE4Parse's list
// in FScriptStruct, without its game-specific entries. Fixed-size ones only need their size.
enum class Native : std::uint8_t {
    Fixed,
    SoftObjectPath,
    GameplayTagContainer,
    PerPlatformBool,
    PerPlatformInt32,     // PerPlatformInt, PerPlatformFloat (4-byte values)
    PerPlatformFrameRate,
    PerPlatformFString,
    PerQualityLevel,
    ExpressionInput,
    ScalarMaterialInput,
    VectorMaterialInput,
    Vector2MaterialInput,
    ColorMaterialInput,
    SmartName,
    NameCurveKey,
    StringCurveKey,
    InstancedStruct,
    Box,                  // LWC: 2 FVector + byte
    SamplingLodBuiltData,
    SamplingRegionBuiltData,
};

struct NativeStruct {
    Native kind;
    std::uint32_t size;   // for Fixed
};

struct NameHash {
    using is_avalanching = void;
    std::uint64_t operator()(std::uint64_t hash) const noexcept { return hash; }
};

const ankerl::unordered_dense::map<std::uint64_t, NativeStruct, NameHash>& getNativeStructs() {
    static const auto table = [] {
        ankerl::unordered_dense::map<std::uint64_t, NativeStruct, NameHash> map;
        auto fixed = [&](const char* name, std::uint32_t size) { map[hashName(name)] = {Native::Fixed, size}; };
        auto special = [&](const char* name, Native kind) { map[hashName(name)] = {kind, 0}; };
        // UE5 math types are doubles (large world coordinates); the *3f/*4f variants are floats.
        fixed("Vector", 24);
        fixed("Vector2D", 16);
        fixed("Vector4", 32);
        fixed("Rotator", 24);
        fixed("Quat", 32);
        fixed("Plane", 32);
        fixed("Sphere", 32);
        fixed("Box2D", 33);
        fixed("Box2f", 17);
        fixed("Box3f", 25);
        fixed("TwoVectors", 48);
        fixed("Matrix", 128);
        fixed("Matrix44f", 64);
        fixed("Vector2f", 8);
        fixed("DeprecateSlateVector2D", 8);
        fixed("Vector3f", 12);
        fixed("Vector3d", 24);
        fixed("Vector4f", 16);
        fixed("Vector4d", 32);
        fixed("Rotator3f", 12);
        fixed("Rotator3d", 24);
        fixed("Quat4f", 16);
        fixed("Quat4d", 32);
        fixed("Plane4f", 16);
        fixed("Plane4d", 32);
        fixed("Sphere3f", 16);
        fixed("Sphere3d", 32);
        fixed("Transform3f", 80);   // as CUE4Parse reads it: FTransform with double components
        fixed("Color", 4);
        fixed("LinearColor", 16);
        fixed("Guid", 16);
        fixed("DateTime", 8);
        fixed("Timespan", 8);
        fixed("FrameNumber", 4);
        fixed("NavAgentSelector", 4);
        fixed("RichCurveKey", 27);
        fixed("SimpleCurveKey", 8);
        fixed("IntPoint", 8);
        fixed("Int32Point", 8);
        fixed("IntVector2", 8);
        fixed("Int32Vector2", 8);
        fixed("UintVector2", 8);
        fixed("Uint32Point", 8);
        fixed("IntVector", 12);
        fixed("UintVector", 12);
        fixed("IntVector4", 16);
        fixed("UintVector4", 16);
        fixed("Uint32Vector4", 16);
        fixed("Int64Vector2", 16);
        fixed("Int64Point", 16);
        fixed("UInt64Vector2", 16);
        fixed("UInt64Point", 16);
        fixed("Int64Vector", 24);
        fixed("UInt64Vector", 24);
        fixed("Int64Vector4", 32);
        fixed("UInt64Vector4", 32);
        fixed("MovieSceneEvaluationKey", 12);
        fixed("MovieSceneFrameRange", 32);
        fixed("MovieSceneSegmentIdentifier", 4);
        fixed("MovieSceneSequenceID", 4);
        fixed("MovieSceneTrackIdentifier", 4);
        fixed("UInt32Property", 4);
        // FKeyHandleMap serializes only when transacting in the editor: nothing in cooked data.
        fixed("KeyHandleMap", 0);
        fixed("ZeroSizeProperty", 0);
        special("Box", Native::Box);
        special("SoftObjectPath", Native::SoftObjectPath);
        special("SoftClassPath", Native::SoftObjectPath);
        special("StringAssetReference", Native::SoftObjectPath);
        special("StringClassReference", Native::SoftObjectPath);
        special("GameplayTagContainer", Native::GameplayTagContainer);
        special("PerPlatformBool", Native::PerPlatformBool);
        special("PerPlatformInt", Native::PerPlatformInt32);
        special("PerPlatformFloat", Native::PerPlatformInt32);
        special("PerPlatformFrameRate", Native::PerPlatformFrameRate);
        special("PerPlatformFString", Native::PerPlatformFString);
        special("PerQualityLevelInt", Native::PerQualityLevel);
        special("PerQualityLevelFloat", Native::PerQualityLevel);
        special("ExpressionInput", Native::ExpressionInput);
        special("MaterialAttributesInput", Native::ExpressionInput);
        special("ScalarMaterialInput", Native::ScalarMaterialInput);
        special("VectorMaterialInput", Native::VectorMaterialInput);
        special("Vector2MaterialInput", Native::Vector2MaterialInput);
        special("ColorMaterialInput", Native::ColorMaterialInput);
        special("SmartName", Native::SmartName);
        special("NameCurveKey", Native::NameCurveKey);
        special("StringCurveKey", Native::StringCurveKey);
        special("InstancedStruct", Native::InstancedStruct);
        special("SkeletalMeshSamplingLODBuiltData", Native::SamplingLodBuiltData);
        special("SkeletalMeshSamplingRegionBuiltData", Native::SamplingRegionBuiltData);
        return map;
    }();
    return table;
}

void skipString(ByteReader& reader) {
    bool wide = false;
    reader.readStringBytes(wide);
}

// FUtf8String: a byte count, then the bytes, with no terminator.
void skipUtf8String(ByteReader& reader) { reader.skip(reader.readCount(1)); }

class Scanner {
public:
    Scanner(ByteReader& reader, const PropertyContext& context) : reader(reader), context(context) {}

    // Reads the properties of a struct and gives each to the sink, when there is one. Stops when the sink returns false.
    Result<void> readStruct(const StructSchema& schema, TagSink* sink, int depth);
    Result<void> skipValue(const PropertyType& type, ReadContext where, int depth);

private:
    Result<void> skipNative(const NativeStruct& native, int depth);
    Result<void> skipText(int depth);
    Result<void> skipFormatArgument(bool isArgumentData, int depth);
    void skipName() { readName(reader, context); }

    Result<void> check() {
        if (reader.hasFailed()) return std::unexpected(reader.getError());
        return {};
    }

    ByteReader& reader;
    const PropertyContext& context;
};

Result<void> Scanner::readStruct(const StructSchema& schema, TagSink* sink, int depth) {
    if (depth > maxDepth) return fail(ErrorCode::InvalidData, "readUnversionedProperties.depth", reader.getPosition());

    // The header: fragments until the last one, then the zero mask of the fragments that have zeroes.
    Fragment fragments[64];
    std::size_t fragmentCount = 0;
    std::uint32_t zeroMaskBits = 0;
    std::uint32_t valueCount = 0;
    for (;;) {
        if (fragmentCount == sizeof(fragments) / sizeof(fragments[0]))
            return fail(ErrorCode::InvalidData, "readUnversionedProperties.fragments", reader.getPosition());
        Fragment fragment = unpackFragment(reader.read<std::uint16_t>());
        if (reader.hasFailed()) return std::unexpected(reader.getError());
        fragments[fragmentCount++] = fragment;
        valueCount += fragment.valueNum;
        if (fragment.hasAnyZeroes) zeroMaskBits += fragment.valueNum;
        if (fragment.isLast) break;
    }
    std::uint32_t zeroMask[64] = {};
    if (zeroMaskBits > 0) {
        if (zeroMaskBits <= 8) {
            zeroMask[0] = reader.read<std::uint8_t>();
        } else if (zeroMaskBits <= 16) {
            zeroMask[0] = reader.read<std::uint16_t>();
        } else {
            std::uint32_t words = (zeroMaskBits + 31) / 32;
            if (words > sizeof(zeroMask) / sizeof(zeroMask[0]))
                return fail(ErrorCode::InvalidData, "readUnversionedProperties.zeroMask", reader.getPosition());
            for (std::uint32_t i = 0; i < words; i++) zeroMask[i] = reader.read<std::uint32_t>();
        }
        if (reader.hasFailed()) return std::unexpected(reader.getError());
    }
    if (valueCount == 0) return {};

    // The values, in schema order.
    std::uint32_t schemaIndex = 0;
    std::uint32_t zeroIndex = 0;
    for (std::size_t f = 0; f < fragmentCount; f++) {
        const Fragment& fragment = fragments[f];
        schemaIndex += fragment.skipNum;
        for (std::uint32_t v = 0; v < fragment.valueNum; v++, schemaIndex++) {
            bool zero = false;
            if (fragment.hasAnyZeroes) {
                zero = (zeroMask[zeroIndex / 32] >> (zeroIndex % 32)) & 1;
                zeroIndex++;
            }
            const PropertyInfo* property = findProperty(schema, schemaIndex);
            static const bool trace = std::getenv("UE5PARSE_TRACE") != nullptr;
            if (trace)
                std::fprintf(stderr, "%*s%s[%u] %s at %zu%s\n", depth * 2, "", std::string(schema.name.getText()).c_str(), schemaIndex,
                             property != nullptr ? std::string(property->name.getText()).c_str() : "?", reader.getPosition(), zero ? " zero" : "");
            if (property == nullptr) {
                if (zero) continue;   // as CUE4Parse: an unknown zero property has no bytes, so it can be passed
                return fail(ErrorCode::MissingProperty, "readUnversionedProperties.unknown", reader.getPosition());
            }
            std::size_t start = reader.getPosition();
            if (!zero) {
                Result<void> skipped = skipValue(*property->type, ReadContext::Normal, depth + 1);
                if (!skipped) return skipped;
            }
            if (sink != nullptr) {
                FPropertyTag tag;
                tag.name = property->name;
                tag.type = property->type;
                tag.offset = static_cast<std::uint32_t>(start);
                tag.size = static_cast<std::uint32_t>(reader.getPosition() - start);
                tag.arrayIndex = property->arrayIndex;
                tag.zero = zero;
                if (!sink->add(tag)) return {};
            }
        }
    }
    return {};
}

Result<void> Scanner::skipValue(const PropertyType& type, ReadContext where, int depth) {
    if (depth > maxDepth) return fail(ErrorCode::InvalidData, "skipPropertyValue.depth", reader.getPosition());
    switch (type.type) {
    case EPropertyType::BoolProperty:
    case EPropertyType::ByteProperty:
    case EPropertyType::Int8Property:
        reader.skip(1);
        break;
    case EPropertyType::Int16Property:
    case EPropertyType::UInt16Property:
        reader.skip(2);
        break;
    case EPropertyType::IntProperty:
    case EPropertyType::UInt32Property:
    case EPropertyType::FloatProperty:
    case EPropertyType::ObjectProperty:
    case EPropertyType::ClassProperty:
    case EPropertyType::VerseDynamicProperty:
    case EPropertyType::InterfaceProperty:
        reader.skip(4);
        break;
    case EPropertyType::Int64Property:
    case EPropertyType::UInt64Property:
    case EPropertyType::DoubleProperty:
        reader.skip(8);
        break;
    case EPropertyType::NameProperty:
        skipName();
        break;
    case EPropertyType::EnumProperty:
        // A top-level enum is its underlying integer; inside a container UE's SerializeItem writes the name.
        if (where == ReadContext::Normal && type.inner != nullptr) return skipValue(*type.inner, where, depth + 1);
        if (where == ReadContext::Normal) reader.skip(1);
        else skipName();
        break;
    case EPropertyType::StrProperty:
    case EPropertyType::VerseStringProperty:
        skipString(reader);
        break;
    case EPropertyType::Utf8StrProperty:
        skipUtf8String(reader);
        break;
    case EPropertyType::AnsiStrProperty:
        reader.skip(reader.readCount(1));
        break;
    case EPropertyType::TextProperty:
        return skipText(depth + 1);
    case EPropertyType::WeakObjectProperty:
        reader.skip(4);   // FPackageIndex, as an ObjectProperty
        break;
    case EPropertyType::LazyObjectProperty:
        reader.skip(16);   // FUniqueObjectGuid
        break;
    case EPropertyType::AssetObjectProperty:
        skipString(reader);
        break;
    case EPropertyType::SoftObjectProperty:
    case EPropertyType::SoftClassProperty:
        return skipNative(NativeStruct{Native::SoftObjectPath, 0}, depth + 1);
    case EPropertyType::DelegateProperty:
    case EPropertyType::VerseFunctionProperty:
        reader.skip(4);   // FPackageIndex object
        skipName();       // function
        break;
    case EPropertyType::MulticastDelegateProperty:
    case EPropertyType::MulticastInlineDelegateProperty: {
        std::uint32_t count = reader.readCount(12);
        for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
            reader.skip(4);
            skipName();
        }
        break;
    }
    case EPropertyType::FieldPathProperty: {
        std::uint32_t count = reader.readCount(8);
        for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) skipName();
        reader.skip(4);   // the owner FPackageIndex
        break;
    }
    case EPropertyType::ArrayProperty: {
        if (type.inner == nullptr) return fail(ErrorCode::InvalidData, "skipPropertyValue.inner", reader.getPosition());
        std::uint32_t count = reader.readCount(0);
        if (!reader.hasFailed() && count > reader.getRemaining())
            return fail(ErrorCode::ReadPastEnd, "skipPropertyValue.arrayCount", reader.getPosition());
        for (std::uint32_t i = 0; i < count; i++) {
            Result<void> element = skipValue(*type.inner, ReadContext::Array, depth + 1);
            if (!element) return element;
        }
        break;
    }
    case EPropertyType::SetProperty: {
        if (type.inner == nullptr) return fail(ErrorCode::InvalidData, "skipPropertyValue.inner", reader.getPosition());
        // Elements to remove (always empty in cooked data), then the elements.
        for (int part = 0; part < 2; part++) {
            std::uint32_t count = reader.readCount(0);
            if (!reader.hasFailed() && count > reader.getRemaining())
                return fail(ErrorCode::ReadPastEnd, "skipPropertyValue.setCount", reader.getPosition());
            for (std::uint32_t i = 0; i < count; i++) {
                Result<void> element = skipValue(*type.inner, ReadContext::Array, depth + 1);
                if (!element) return element;
            }
        }
        break;
    }
    case EPropertyType::MapProperty: {
        if (type.inner == nullptr || type.value == nullptr)
            return fail(ErrorCode::InvalidData, "skipPropertyValue.inner", reader.getPosition());
        std::uint32_t removed = reader.readCount(0);
        if (!reader.hasFailed() && removed > reader.getRemaining())
            return fail(ErrorCode::ReadPastEnd, "skipPropertyValue.mapCount", reader.getPosition());
        for (std::uint32_t i = 0; i < removed; i++) {
            Result<void> key = skipValue(*type.inner, ReadContext::Map, depth + 1);
            if (!key) return key;
        }
        std::uint32_t count = reader.readCount(0);
        if (!reader.hasFailed() && count > reader.getRemaining())
            return fail(ErrorCode::ReadPastEnd, "skipPropertyValue.mapCount", reader.getPosition());
        for (std::uint32_t i = 0; i < count; i++) {
            Result<void> key = skipValue(*type.inner, ReadContext::Map, depth + 1);
            if (!key) return key;
            Result<void> value = skipValue(*type.value, ReadContext::Map, depth + 1);
            if (!value) return value;
        }
        break;
    }
    case EPropertyType::OptionalProperty: {
        if (type.inner == nullptr) return fail(ErrorCode::InvalidData, "skipPropertyValue.inner", reader.getPosition());
        bool isSet = reader.read<std::int32_t>() != 0;
        if (isSet) return skipValue(*type.inner, ReadContext::Optional, depth + 1);
        break;
    }
    case EPropertyType::StructProperty: {
        if (type.structSchema != nullptr) return readStruct(*type.structSchema, nullptr, depth + 1);
        auto found = getNativeStructs().find(type.structName.getHash());
        if (found != getNativeStructs().end()) return skipNative(found->second, depth + 1);
        const StructSchema* schema = context.mappings != nullptr ? context.mappings->findStruct(type.structName) : nullptr;
        if (schema == nullptr) return fail(ErrorCode::MissingSchema, "skipPropertyValue.struct", reader.getPosition());
        return readStruct(*schema, nullptr, depth + 1);
    }
    default:
        return fail(ErrorCode::UnknownPropertyType, "skipPropertyValue.type", reader.getPosition());
    }
    return check();
}

Result<void> Scanner::skipNative(const NativeStruct& native, int depth) {
    EngineVersion engine = context.engine;
    switch (native.kind) {
    case Native::Fixed:
        reader.skip(native.size);
        break;
    case Native::Box:
        reader.skip(24 + 24 + 1);
        break;
    case Native::SoftObjectPath:
        // FTopLevelAssetPath (package and asset names) since 5.1; one name before. Then the sub path.
        skipName();
        if (engine >= EngineVersion::UE5_1) skipName();
        if (engine >= EngineVersion::UE5_6) skipUtf8String(reader);
        else skipString(reader);
        break;
    case Native::GameplayTagContainer: {
        std::uint32_t count = reader.readCount(8);
        for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) skipName();
        break;
    }
    case Native::PerPlatformBool:
    case Native::PerPlatformInt32:
    case Native::PerPlatformFrameRate:
    case Native::PerPlatformFString: {
        // bCooked, the default; cooked data has no per-platform map before 5.8 (CUE4Parse: !bCooked and 5.8+).
        bool cooked = reader.read<std::int32_t>() != 0;
        auto skipOne = [&] {
            if (native.kind == Native::PerPlatformBool || native.kind == Native::PerPlatformInt32) reader.skip(4);
            else if (native.kind == Native::PerPlatformFrameRate) reader.skip(8);
            else skipString(reader);
        };
        skipOne();
        if (!cooked && engine >= EngineVersion::UE5_8) {
            std::uint32_t count = reader.readCount(8);
            for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
                skipName();
                skipOne();
            }
        }
        break;
    }
    case Native::PerQualityLevel: {
        reader.skip(4 + 4);   // bCooked, the default
        std::uint32_t count = reader.readCount(8);
        reader.skip(std::size_t(count) * 8);
        break;
    }
    case Native::ExpressionInput:
    case Native::ScalarMaterialInput:
    case Native::VectorMaterialInput:
    case Native::Vector2MaterialInput:
    case Native::ColorMaterialInput:
        // FExpressionInput: Expression (5.1+ in cooked data), OutputIndex, InputName, the five masks; before 5.2 the
        // expression name. A material input adds UseConstant and the constant.
        if (engine >= EngineVersion::UE5_1) reader.skip(4);
        reader.skip(4);
        skipName();
        reader.skip(5 * 4);
        if (engine < EngineVersion::UE5_2) skipName();
        if (native.kind == Native::ScalarMaterialInput) reader.skip(4 + 4);
        else if (native.kind == Native::VectorMaterialInput) reader.skip(4 + 12);
        else if (native.kind == Native::Vector2MaterialInput) reader.skip(4 + 8);
        else if (native.kind == Native::ColorMaterialInput) reader.skip(4 + (engine >= EngineVersion::UE5_6 ? 16 : 4));
        break;
    case Native::SmartName:
        skipName();
        break;
    case Native::NameCurveKey:
        reader.skip(4);   // Time
        skipName();
        break;
    case Native::StringCurveKey:
        reader.skip(4);
        skipString(reader);
        break;
    case Native::InstancedStruct: {
        if (engine < EngineVersion::UE5_3) {
            // The legacy editor header is optional; then one version byte.
            std::size_t at = reader.getPosition();
            if (reader.read<std::uint32_t>() != 0xABABABABu && !reader.hasFailed()) reader.seek(at);
            reader.skip(1);
        }
        reader.skip(4);   // the struct FPackageIndex
        std::int32_t size = reader.read<std::int32_t>();
        if (size < 0) return fail(ErrorCode::InvalidData, "skipPropertyValue.instancedStruct", reader.getPosition());
        reader.skip(static_cast<std::size_t>(size));
        break;
    }
    case Native::SamplingLodBuiltData:
    case Native::SamplingRegionBuiltData: {
        // FWeightedRandomSampler: Prob (floats), Alias (ints), TotalWeight; a region has triangle and bone indices
        // before it and vertices after it.
        auto skipIntArray = [&] { reader.skip(std::size_t(reader.readCount(4)) * 4); };
        if (native.kind == Native::SamplingRegionBuiltData) {
            skipIntArray();
            skipIntArray();
        }
        skipIntArray();
        skipIntArray();
        reader.skip(4);
        if (native.kind == Native::SamplingRegionBuiltData) skipIntArray();
        break;
    }
    }
    (void)depth;
    return check();
}

Result<void> Scanner::skipFormatArgument(bool isArgumentData, int depth) {
    // EFormatArgumentType: Int, UInt, Float, Double, Text, Gender.
    std::uint8_t type = reader.read<std::uint8_t>();
    switch (type) {
    case 0: reader.skip(8); break;   // Int: 64-bit since 5.0 for argument data too
    case 1: reader.skip(8); break;
    case 2: reader.skip(4); break;
    case 3: reader.skip(8); break;
    case 4: return skipText(depth + 1);
    default: return fail(ErrorCode::InvalidData, "skipPropertyValue.formatArgument", reader.getPosition());
    }
    (void)isArgumentData;
    return check();
}

Result<void> Scanner::skipText(int depth) {
    if (depth > maxDepth) return fail(ErrorCode::InvalidData, "skipPropertyValue.textDepth", reader.getPosition());
    reader.skip(4);   // flags
    std::int8_t history = reader.read<std::int8_t>();
    switch (history) {
    case -1:   // None: an optional culture invariant string
        if (reader.read<std::int32_t>() != 0) skipString(reader);
        break;
    case 0:   // Base: namespace, key, source string
        skipString(reader);
        skipString(reader);
        skipString(reader);
        break;
    case 1:   // NamedFormat: the format text, then a map of name -> argument
    case 2:   // OrderedFormat: the format text, then an array of arguments
    case 3: { // ArgumentFormat: the format text, then an array of (name, argument data)
        Result<void> format = skipText(depth + 1);
        if (!format) return format;
        std::uint32_t count = reader.readCount(1);
        for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
            if (history != 2) skipString(reader);
            Result<void> argument = skipFormatArgument(history == 3, depth + 1);
            if (!argument) return argument;
        }
        break;
    }
    case 4:   // AsNumber
    case 5:   // AsPercent
    case 6: { // AsCurrency: the currency code first
        if (history == 6) skipString(reader);
        Result<void> value = skipFormatArgument(false, depth + 1);
        if (!value) return value;
        if (reader.read<std::int32_t>() != 0) {
            // FNumberFormattingOptions: AlwaysSign, UseGrouping, RoundingMode, four digit counts.
            reader.skip(4 + 4 + 1 + 4 * 4);
        }
        skipString(reader);   // target culture
        break;
    }
    case 7:   // AsDate: date time, date style, time zone, culture
        reader.skip(8 + 1);
        skipString(reader);
        skipString(reader);
        break;
    case 8:   // AsTime: date time, time style, time zone, culture
        reader.skip(8 + 1);
        skipString(reader);
        skipString(reader);
        break;
    case 9:   // AsDateTime: date time, date style, time style, time zone, culture
        reader.skip(8 + 1 + 1);
        skipString(reader);
        skipString(reader);
        break;
    case 10: { // Transform: the source text, then the transform type
        Result<void> source = skipText(depth + 1);
        if (!source) return source;
        reader.skip(1);
        break;
    }
    case 11:   // StringTableEntry: the table id, the key
        skipName();
        skipString(reader);
        break;
    case 12:   // TextGenerator: the generator type id (CUE4Parse reads nothing more)
        skipName();
        break;
    default:
        return fail(ErrorCode::InvalidData, "skipPropertyValue.textHistory", reader.getPosition());
    }
    return check();
}

}

bool isNativeStruct(FName structName) { return getNativeStructs().contains(structName.getHash()); }

FName readName(ByteReader& reader, const PropertyContext& context) {
    std::uint32_t index = reader.read<std::uint32_t>();
    std::uint32_t number = reader.read<std::uint32_t>();
    if (reader.hasFailed()) return FName();
    if (index >= context.names.size()) {
        reader.fail(ErrorCode::InvalidData, "readName.index");
        return FName();
    }
    const FName& base = context.names[index];
    return FName(base.getText(), base.getHash(), number);
}

namespace {

class VectorSink final : public TagSink {
public:
    explicit VectorSink(std::vector<FPropertyTag>& out) : out(out) {}
    bool add(const FPropertyTag& tag) override {
        out.push_back(tag);
        return true;
    }

private:
    std::vector<FPropertyTag>& out;
};

class FindSink final : public TagSink {
public:
    FindSink(FName name, std::uint32_t arrayIndex) : name(name), arrayIndex(arrayIndex) {}
    bool add(const FPropertyTag& tag) override {
        if (tag.name != name || tag.arrayIndex != arrayIndex) return true;
        found = tag;
        return false;
    }

    FName name;
    std::uint32_t arrayIndex;
    std::optional<FPropertyTag> found;
};

}

Result<void> readUnversionedProperties(ByteReader& reader, const PropertyContext& context, const StructSchema& schema,
                                       TagSink& sink) {
    Scanner scanner(reader, context);
    return scanner.readStruct(schema, &sink, 0);
}

Result<void> readUnversionedProperties(ByteReader& reader, const PropertyContext& context, const StructSchema& schema,
                                       std::vector<FPropertyTag>& out) {
    VectorSink sink(out);
    return readUnversionedProperties(reader, context, schema, sink);
}

Result<std::optional<FPropertyTag>> findUnversionedProperty(ByteReader& reader, const PropertyContext& context,
                                                            const StructSchema& schema, FName name, std::uint32_t arrayIndex) {
    FindSink sink(name, arrayIndex);
    Result<void> read = readUnversionedProperties(reader, context, schema, sink);
    if (!read) return std::unexpected(read.error());
    return sink.found;
}

Result<void> skipPropertyValue(ByteReader& reader, const PropertyContext& context, const PropertyType& type, ReadContext where) {
    Scanner scanner(reader, context);
    return scanner.skipValue(type, where, 0);
}

}
