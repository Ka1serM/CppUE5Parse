#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SerializedProperties.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"
#include "Engine/Source/Runtime/Engine/StaticParameterSet.h"
#include "Test.h"

namespace {

namespace rt = ue5parse::runtime;

using FTypeName = std::initializer_list<std::pair<std::string, std::int32_t>>;

constexpr std::uint8_t BoolTrue = 0x10;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    void putString(std::string_view text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        for (char c : text)
            put(c);
        put('\0');
    }

    void putBytes(const Writer& other) { bytes.insert(bytes.end(), other.bytes.begin(), other.bytes.end()); }

    void putTag(std::string_view name, FTypeName type, const Writer& value, std::uint8_t flags = 0) {
        putString(name);
        for (const auto& [node, innerCount] : type) {
            putString(node);
            put<std::int32_t>(innerCount);
        }
        put<std::int32_t>(static_cast<std::int32_t>(value.bytes.size()));
        put<std::uint8_t>(flags);
        putBytes(value);
    }

    void putNone() { putString("None"); }

    std::vector<std::byte> bytes;
};

class RecordingArchive : public rt::FMemoryReaderView {
public:
    RecordingArchive(std::span<const std::byte> bytes, rt::FSerializedProperties& store) : FMemoryReaderView(bytes), store(store) {
        SetGame(ue5parse::EGame::GAME_UE5_4);
        SetUseUnversionedPropertySerialization(false);
        SetUEVer(ue5parse::getPackageFileVersion(ue5parse::EGame::GAME_UE5_4));
    }

    using FMemoryReaderView::operator<<;
    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = rt::FName(Text.ToUtf8());
        return *this;
    }
    rt::FSerializedProperties* GetSerializedProperties() override { return &store; }

private:
    rt::FSerializedProperties& store;
};

ue5parse::MountedGame emptyGame() {
    ue5parse::MountOptions options;
    options.game = ue5parse::EGame::GAME_UE5_4;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

const rt::FProperty* findProperty(const rt::UStruct* type, const char* name) {
    return type->FindPropertyByName(rt::FName(name));
}

std::string nameOf(const rt::UStruct* type) {
    return type->GetName().ToUtf8();
}

Writer infoWithIndex(std::int32_t index) {
    Writer value;
    value.put(index);
    Writer w;
    w.putTag("Index", {{"IntProperty", 0}}, value);
    w.putNone();
    return w;
}

Writer infoWithName(std::string_view text) {
    Writer value;
    value.putString(text);
    Writer w;
    w.putTag("Name", {{"NameProperty", 0}}, value);
    w.putNone();
    return w;
}

}

TEST(serializedPropertiesKeepNestedStructsAndArrayElements) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const rt::UScriptStruct* root = rt::FStaticParameterSetRuntimeData::StaticStruct();
    const rt::UScriptStruct* parameter = rt::FStaticSwitchParameter::StaticStruct();
    const rt::UScriptStruct* info = rt::FMaterialParameterInfo::StaticStruct();
    Writer elements;
    elements.put<std::int32_t>(2);
    elements.putTag("Value", {{"BoolProperty", 0}}, Writer(), BoolTrue);
    elements.putTag("ParameterInfo", {{"StructProperty", 1}, {nameOf(info), 0}}, infoWithIndex(5));
    elements.putNone();
    elements.putNone();
    Writer w;
    w.putTag("StaticSwitchParameters", {{"ArrayProperty", 1}, {"StructProperty", 1}, {nameOf(parameter), 0}}, elements);
    w.putTag("bHasMaterialLayers", {{"BoolProperty", 0}}, Writer());
    w.putNone();
    rt::FStaticParameterSetRuntimeData value;
    rt::FSerializedProperties store(&value);
    RecordingArchive ar(w.bytes, store);
    root->SerializeItem(ar, &value);
    REQUIRE(!ar.IsError());
    CHECK(ar.AtEnd());
    store.Sort();
    REQUIRE(value.StaticSwitchParameters.Num() == 2);
    CHECK(value.StaticSwitchParameters[0].Value);
    CHECK(value.StaticSwitchParameters[0].ParameterInfo.Index == 5);

    const rt::FProperty* switches = findProperty(root, "StaticSwitchParameters");
    CHECK(store.Num() == 2);
    CHECK(store.Contains(switches, 0));
    CHECK(store.Contains(findProperty(root, "bHasMaterialLayers"), 0));
    CHECK(!store.Contains(findProperty(root, "MaterialLayers"), 0));
    CHECK(store.FindChild(findProperty(root, "MaterialLayers"), 0) == nullptr);
    CHECK(store.FindChild(findProperty(root, "bHasMaterialLayers"), 0) == nullptr);

    const rt::FSerializedPropertyList* array = store.FindChild(switches, 0);
    REQUIRE(array != nullptr);
    CHECK(array->Num() == 0);
    const rt::FProperty* inner = rt::CastField<rt::FArrayProperty>(switches)->Inner;
    const rt::FSerializedPropertyList* first = array->FindChild(inner, 0);
    REQUIRE(first != nullptr);
    CHECK(first->Num() == 2);
    CHECK(first->Contains(findProperty(parameter, "Value"), 0));
    CHECK(first->Contains(findProperty(parameter, "ParameterInfo"), 0));
    CHECK(!first->Contains(findProperty(parameter, "bOverride"), 0));
    const rt::FSerializedPropertyList* firstInfo = first->FindChild(findProperty(parameter, "ParameterInfo"), 0);
    REQUIRE(firstInfo != nullptr);
    CHECK(firstInfo->Num() == 1);
    CHECK(firstInfo->Contains(findProperty(info, "Index"), 0));
    CHECK(!firstInfo->Contains(findProperty(info, "Name"), 0));
    const rt::FSerializedPropertyList* second = array->FindChild(inner, 1);
    REQUIRE(second != nullptr);
    CHECK(second->Num() == 0);
    CHECK(array->FindChild(inner, 2) == nullptr);
}

TEST(serializedPropertiesKeepDataTableRowsAndTheLastCopyOfARow) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const rt::UScriptStruct* info = rt::FMaterialParameterInfo::StaticStruct();
    Writer w;
    w.put<std::uint8_t>(0);
    w.putNone();
    w.put<std::uint32_t>(0);
    w.put<std::int32_t>(3);
    w.putString("RowA");
    w.putBytes(infoWithIndex(5));
    w.putString("RowB");
    w.putNone();
    w.putString("RowA");
    w.putBytes(infoWithName("Second"));
    rt::UDataTable* table = rt::NewObject<rt::UDataTable>(rt::GetTransientPackage());
    table->RowStruct = const_cast<rt::UScriptStruct*>(info);
    rt::FSerializedProperties store(table);
    RecordingArchive ar(w.bytes, store);
    table->Serialize(ar);
    REQUIRE(!ar.IsError());
    CHECK(ar.AtEnd());
    store.Sort();
    CHECK(store.Num() == 0);
    const rt::FSerializedPropertyList* rowA = store.FindRow(rt::FName("RowA"));
    REQUIRE(rowA != nullptr);
    CHECK(rowA->Num() == 1);
    CHECK(rowA->Contains(findProperty(info, "Name"), 0));
    CHECK(!rowA->Contains(findProperty(info, "Index"), 0));
    const rt::FSerializedPropertyList* rowB = store.FindRow(rt::FName("RowB"));
    REQUIRE(rowB != nullptr);
    CHECK(rowB->Num() == 0);
    CHECK(store.FindRow(rt::FName("RowC")) == nullptr);
}

TEST(serializedPropertiesKeepTheLastCopyOfATaggedStruct) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const rt::UScriptStruct* parameter = rt::FStaticSwitchParameter::StaticStruct();
    const rt::UScriptStruct* info = rt::FMaterialParameterInfo::StaticStruct();
    Writer w;
    w.putTag("ParameterInfo", {{"StructProperty", 1}, {nameOf(info), 0}}, infoWithIndex(5));
    w.putTag("ParameterInfo", {{"StructProperty", 1}, {nameOf(info), 0}}, infoWithName("Second"));
    w.putNone();
    rt::FStaticSwitchParameter value;
    rt::FSerializedProperties store(&value);
    RecordingArchive ar(w.bytes, store);
    parameter->SerializeItem(ar, &value);
    REQUIRE(!ar.IsError());
    CHECK(ar.AtEnd());
    store.Sort();
    CHECK(value.ParameterInfo.Name == rt::FName("Second"));
    const rt::FSerializedPropertyList* last = store.FindChild(findProperty(parameter, "ParameterInfo"), 0);
    REQUIRE(last != nullptr);
    CHECK(last->Num() == 1);
    CHECK(last->Contains(findProperty(info, "Name"), 0));
}

namespace {

class SchemaOfProperties {
public:
    explicit SchemaOfProperties(const rt::UStruct* type) : type(const_cast<rt::UStruct*>(type)), saved(this->type->UnversionedSchema) {
        this->type->UnversionedSchema.Empty();
        for (const rt::UStruct* level = type; level != nullptr; level = level->GetSuperStruct())
            for (rt::FField* field = level->ChildProperties; field != nullptr; field = field->Next)
                if (rt::FProperty* property = rt::CastField<rt::FProperty>(field))
                    this->type->UnversionedSchema.Add({property, 0});
    }
    ~SchemaOfProperties() { type->UnversionedSchema = saved; }
    SchemaOfProperties(const SchemaOfProperties&) = delete;
    SchemaOfProperties& operator=(const SchemaOfProperties&) = delete;

    std::uint32_t slotOf(const char* name) const {
        const rt::FProperty* property = findProperty(type, name);
        for (rt::int32 i = 0; i < type->UnversionedSchema.Num(); i++)
            if (type->UnversionedSchema[i].Property == property)
                return static_cast<std::uint32_t>(i);
        UE5PARSE_CHECK(false);
        return 0;
    }

private:
    rt::UStruct* type;
    rt::TArray<rt::UStruct::FUnversionedSlot> saved;
};

void putHeader(Writer& w, const SchemaOfProperties& schema, std::initializer_list<std::pair<const char*, bool>> slots) {
    if (slots.size() == 0) {
        w.put<std::uint16_t>(0x100);
        return;
    }
    std::uint32_t next = 0;
    std::uint8_t zeroMask = 0;
    std::uint32_t zeroes = 0;
    std::size_t written = 0;
    for (const auto& [name, bZero] : slots) {
        const std::uint32_t slot = schema.slotOf(name);
        UE5PARSE_CHECK(slot >= next);
        const bool bLast = ++written == slots.size();
        w.put<std::uint16_t>(static_cast<std::uint16_t>((slot - next) | (bZero ? 0x80 : 0) | (bLast ? 0x100 : 0) | (1 << 9)));
        if (bZero)
            zeroMask |= static_cast<std::uint8_t>(1u << zeroes++);
        next = slot + 1;
    }
    if (zeroes > 0)
        w.put(zeroMask);
}

}

TEST(serializedPropertiesKeepUnversionedNestedStructsAndZeroValues) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const rt::UScriptStruct* parameter = rt::FStaticSwitchParameter::StaticStruct();
    const rt::UScriptStruct* info = rt::FMaterialParameterInfo::StaticStruct();
    const SchemaOfProperties parameterSchema(parameter);
    const SchemaOfProperties infoSchema(info);
    Writer w;
    putHeader(w, parameterSchema, {{"Value", false}, {"ParameterInfo", false}, {"bOverride", true}});
    w.put<std::uint8_t>(1);
    putHeader(w, infoSchema, {{"Index", false}});
    w.put<std::int32_t>(5);
    rt::FStaticSwitchParameter value;
    value.bOverride = true;
    rt::FSerializedProperties store(&value);
    RecordingArchive ar(w.bytes, store);
    ar.SetUseUnversionedPropertySerialization(true);
    parameter->SerializeItem(ar, &value);
    REQUIRE(!ar.IsError());
    CHECK(ar.AtEnd());
    store.Sort();
    CHECK(value.Value);
    CHECK(!value.bOverride);
    CHECK(value.ParameterInfo.Index == 5);
    CHECK(store.Num() == 3);
    CHECK(store.Contains(findProperty(parameter, "bOverride"), 0));
    CHECK(!store.Contains(findProperty(parameter, "ExpressionGUID"), 0));
    CHECK(store.FindChild(findProperty(parameter, "bOverride"), 0) == nullptr);
    const rt::FSerializedPropertyList* parameterInfo = store.FindChild(findProperty(parameter, "ParameterInfo"), 0);
    REQUIRE(parameterInfo != nullptr);
    CHECK(parameterInfo->Num() == 1);
    CHECK(parameterInfo->Contains(findProperty(info, "Index"), 0));
}

TEST(serializedPropertiesSkipStructsReadAtAnotherAddress) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const rt::UScriptStruct* root = rt::FStaticParameterSetRuntimeData::StaticStruct();
    const rt::UScriptStruct* parameter = rt::FStaticSwitchParameter::StaticStruct();
    const rt::UScriptStruct* info = rt::FMaterialParameterInfo::StaticStruct();
    const rt::FProperty* materialLayers = findProperty(root, "MaterialLayers");
    Writer w;
    w.putTag("ParameterInfo", {{"StructProperty", 1}, {nameOf(info), 0}}, infoWithIndex(5));
    w.putNone();
    rt::FStaticParameterSetRuntimeData value;
    rt::FStaticSwitchParameter other;
    rt::FSerializedProperties store(&value);
    RecordingArchive ar(w.bytes, store);
    {
        rt::FSerializedPropertyScope scope(ar, &value, materialLayers, 0);
        parameter->SerializeItem(ar, &other);
    }
    REQUIRE(!ar.IsError());
    CHECK(ar.AtEnd());
    store.Sort();
    CHECK(other.ParameterInfo.Index == 5);
    CHECK(store.Num() == 0);
    CHECK(store.FindChild(materialLayers, 0) == nullptr);
}

TEST(serializedPropertiesSkipStructsInsideAValueThatIsNotATaggedStruct) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const rt::UScriptStruct* root = rt::FStaticParameterSetRuntimeData::StaticStruct();
    const rt::UScriptStruct* parameter = rt::FStaticSwitchParameter::StaticStruct();
    const rt::UScriptStruct* info = rt::FMaterialParameterInfo::StaticStruct();
    const rt::FProperty* materialLayers = findProperty(root, "MaterialLayers");
    REQUIRE(materialLayers->GetOffset_ForInternal() == 0);
    Writer w;
    w.putTag("ParameterInfo", {{"StructProperty", 1}, {nameOf(info), 0}}, infoWithIndex(5));
    w.putNone();
    rt::FStaticSwitchParameter value;
    rt::FSerializedProperties store(&value);
    RecordingArchive ar(w.bytes, store);
    {
        rt::FSerializedPropertyScope scope(ar, &value, materialLayers, 0);
        parameter->SerializeItem(ar, &value);
    }
    REQUIRE(!ar.IsError());
    store.Sort();
    CHECK(value.ParameterInfo.Index == 5);
    CHECK(store.Num() == 0);
    CHECK(store.FindChild(materialLayers, 0) == nullptr);
}
