#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Test.h"

namespace {

namespace rt = ue5parse::runtime;
using ue5parse::EGame;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    void string(std::string_view text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        for (char c : text)
            put(c);
        put('\0');
    }

    std::vector<std::byte> bytes;
};

class BagArchive : public rt::FMemoryReaderView {
public:
    using rt::FMemoryReaderView::FMemoryReaderView;
    using rt::FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = rt::MakeName(Text);
        return *this;
    }

    rt::FArchive& operator<<(rt::UObject*& Value) override {
        std::int32_t Index = 0;
        Serialize(&Index, sizeof(Index));
        Value = nullptr;
        return *this;
    }
};

void writeDesc(Writer& out, std::uint32_t id, std::string_view name, std::uint8_t valueType, bool withMetaData) {
    out.put<std::int32_t>(0);
    for (std::uint32_t Part : {id, 0x51u, 0x52u, 0x53u})
        out.put(Part);
    out.string(name);
    out.put(valueType);
    out.put<std::uint8_t>(0);
    out.put<std::uint32_t>(withMetaData ? 1 : 0);
    if (!withMetaData)
        return;
    out.put<std::int32_t>(1);
    out.string("ClampMin");
    out.string("0");
    out.put<std::int32_t>(0);
}

void writeBag54(Writer& out) {
    out.put<std::uint32_t>(1);
    out.put<std::int32_t>(3);
    writeDesc(out, 0x40, "Speed", 5, true);
    writeDesc(out, 0x41, "Count", 3, false);
    writeDesc(out, 0x42, "Speed", 3, false);
    out.put<std::int32_t>(2);
    out.put<std::uint16_t>(0x0100);
}

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

}

TEST(propertyBag54KeepsItsDescsWithMetaData) {
    Writer out;
    writeBag54(out);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE5_4));
    rt::FRuntime runtime(provider);
    BagArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_4);
    rt::FInstancedPropertyBag Bag;
    Bag.Serialize(Ar);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    const rt::UPropertyBag* Struct = Bag.GetPropertyBagStruct();
    REQUIRE(Struct != nullptr);
    const rt::TArray<rt::FPropertyBagPropertyDesc>& Descs = Struct->GetPropertyDescs();
    REQUIRE(Descs.Num() == 2);
    CHECK(Descs[0].Name == rt::FName("Speed"));
    CHECK(Descs[0].ValueType == rt::EPropertyBagPropertyType::Float);
    REQUIRE(Descs[0].MetaData.Num() == 1);
    CHECK(Descs[0].MetaData[0].Key == rt::FName("ClampMin"));
    CHECK(Descs[0].MetaData[0].Value.ToUtf8() == "0");
    CHECK(Descs[1].Name == rt::FName("Count"));
    CHECK(Descs[1].MetaData.Num() == 0);
}

TEST(propertyBag53ReadsNoMetaClassAfterMetaData) {
    Writer out;
    writeBag54(out);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE5_3));
    rt::FRuntime runtime(provider);
    BagArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_3);
    rt::FInstancedPropertyBag Bag;
    Bag.Serialize(Ar);
    CHECK(Ar.IsError());
}
