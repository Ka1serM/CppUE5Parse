#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/GameTypes/CodeVein2/CodeVein2StringEncryption.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/Core/Internationalization/TextLocalizationResource.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/Engine/Internationalization/StringTable.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::runtime::FMemoryReaderView;
using ue5parse::runtime::FText;
namespace rt = ue5parse::runtime;

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

    template <typename T>
    void patch(std::size_t at, T value) {
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    std::vector<std::byte> bytes;
};

std::string encodeBase64(std::span<const std::uint8_t> data) {
    constexpr char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (std::size_t i = 0; i < data.size(); i += 3) {
        std::uint32_t chunk = static_cast<std::uint32_t>(data[i]) << 16;
        if (i + 1 < data.size())
            chunk |= static_cast<std::uint32_t>(data[i + 1]) << 8;
        if (i + 2 < data.size())
            chunk |= data[i + 2];
        out.push_back(digits[(chunk >> 18) & 63]);
        out.push_back(digits[(chunk >> 12) & 63]);
        out.push_back(i + 1 < data.size() ? digits[(chunk >> 6) & 63] : '=');
        out.push_back(i + 2 < data.size() ? digits[chunk & 63] : '=');
    }
    return out;
}

void writeBaseText(Writer& out, std::string_view source) {
    out.put<std::uint32_t>(0);
    out.put<std::int8_t>(0);
    out.string("Namespace");
    out.string("Key");
    out.string(source);
}

}

TEST(splitgate2TextSkipsTrailingBytes) {
    Writer out;
    writeBaseText(out, "Hello");
    out.put<std::uint32_t>(0xFFFFFFFF);
    out.put<std::uint32_t>(0x12345678);
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_Splitgate2);
    FText text;
    reader << text;
    REQUIRE(!reader.IsError());
    CHECK(text.ToString().ToUtf8() == "Hello");
    std::uint32_t next = 0;
    reader << next;
    CHECK(next == 0x12345678);
}

TEST(embersOfTheUncrownedTextDecryptsSource) {
    constexpr std::string_view key = "BM8c/12uWU7Z78MbVza5A6MrCeoTIaS6nIbXjFQNlrs=";
    std::vector<std::uint8_t> stored = {'H', 0, 'i', 0, 0, 0};
    for (std::size_t i = 0; i < stored.size(); i++)
        stored[i] ^= static_cast<std::uint8_t>(key[i % key.size()]);
    Writer out;
    writeBaseText(out, encodeBase64(stored) + "ENCRYPTED");
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_EmbersofTheUncrowned);
    FText text;
    reader << text;
    REQUIRE(!reader.IsError());
    CHECK(text.ToString().ToUtf8() == "Hi");
}

TEST(embersOfTheUncrownedTextRejectsBadBase64) {
    Writer out;
    writeBaseText(out, "!!!!ENCRYPTED");
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_EmbersofTheUncrowned);
    FText text;
    reader << text;
    CHECK(reader.IsError());
}

namespace {

void writeCodeVein2String(Writer& out, std::string_view text, ue5parse::CodeVein2StringKey key) {
    std::vector<std::uint8_t> bytes(text.begin(), text.end());
    bytes.push_back(0);
    ue5parse::decryptCodeVein2String(std::span<std::uint8_t>(bytes), key);
    out.put<std::int32_t>(static_cast<std::int32_t>(bytes.size()));
    for (std::uint8_t byte : bytes)
        out.put(byte);
}

void writeLocresHeader(Writer& out, std::uint8_t version) {
    out.put<std::uint32_t>(0x7574140E);
    out.put<std::uint32_t>(0xFC034A67);
    out.put<std::uint32_t>(0x9D90154A);
    out.put<std::uint32_t>(0x1B7F37C3);
    out.put(version);
}

const rt::FTextLocalizationResource::FEntry* findEntry(const rt::FTextLocalizationResource& resource, std::string_view space, std::string_view key) {
    return resource.FindEntry(rt::FTextId{rt::FString(space), rt::FString(key)});
}

}

TEST(codeVein2StringDecryptsNarrowCharacters) {
    Writer out;
    writeCodeVein2String(out, "Hello", ue5parse::CodeVein2StringKey::Locres);
    CHECK(static_cast<char>(out.bytes[4]) != 'H');
    FMemoryReaderView reader(out.bytes);
    CHECK(rt::SerializeCodeVein2String(reader, ue5parse::CodeVein2StringKey::Locres).ToUtf8() == "Hello");
    CHECK(!reader.IsError());
    CHECK(reader.AtEnd());
}

TEST(codeVein2StringMapsWideMarkers) {
    std::array<std::uint8_t, 3> keyBytes = {0, 0, 0};
    ue5parse::decryptCodeVein2String(std::span<std::uint8_t>(keyBytes), ue5parse::CodeVein2StringKey::StringTable);
    CHECK(keyBytes[0] == 0xD7);
    Writer out;
    out.put<std::int32_t>(-3);
    out.put<char16_t>(0xF000);
    out.put<char16_t>(static_cast<char16_t>(u'A' ^ keyBytes[1]));
    out.put<char16_t>(0);
    FMemoryReaderView reader(out.bytes);
    const rt::FString value = rt::SerializeCodeVein2String(reader, ue5parse::CodeVein2StringKey::StringTable);
    REQUIRE(!reader.IsError());
    REQUIRE(value.View().size() == 2);
    CHECK(value.View()[0] == static_cast<char16_t>(keyBytes[0]));
    CHECK(value.View()[1] == u'A');
}

TEST(legacyLocresReadsInlineStrings) {
    Writer out;
    out.put<std::uint32_t>(1);
    out.string("Menu");
    out.put<std::uint32_t>(1);
    out.string("Start");
    out.put<std::uint32_t>(0xAABBCCDD);
    out.string("Play");
    FMemoryReaderView reader(out.bytes);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("Game/Content/Localization/Game/en/Game.locres")));
    const auto* entry = findEntry(resource, "Menu", "Start");
    REQUIRE(entry != nullptr);
    CHECK(entry->LocalizedString.ToUtf8() == "Play");
    CHECK(entry->SourceStringHash == 0xAABBCCDD);
    CHECK(findEntry(resource, "Menu", "start") == nullptr);
    CHECK(findEntry(resource, "menu", "Start") == nullptr);
    CHECK(reader.AtEnd());
}

namespace {

void writeOptimizedLocres(Writer& out, std::uint8_t version, bool stellarBladeExtra) {
    writeLocresHeader(out, version);
    const std::size_t offsetAt = out.bytes.size();
    out.put<std::int64_t>(0);
    out.put<std::uint32_t>(2);
    out.put<std::uint32_t>(1);
    out.put<std::uint32_t>(0x1111);
    out.string("Menu");
    out.put<std::uint32_t>(2);
    for (std::int32_t i = 0; i < 2; i++) {
        out.put<std::uint32_t>(0x2222);
        out.string(i == 0 ? "Start" : "Quit");
        out.put<std::uint32_t>(static_cast<std::uint32_t>(0x3333 + i));
        out.put<std::int32_t>(1 - i);
        if (stellarBladeExtra)
            out.put<std::uint32_t>(0xFFFFFFFF);
    }
    out.patch<std::int64_t>(offsetAt, static_cast<std::int64_t>(out.bytes.size()));
    out.put<std::int32_t>(2);
    out.string("Leave");
    out.put<std::int32_t>(1);
    out.string("Play");
    out.put<std::int32_t>(1);
}

void writeCompactLocres(Writer& out, const std::function<void(Writer&)>& writeStrings) {
    writeLocresHeader(out, 1);
    const std::size_t offsetAt = out.bytes.size();
    out.put<std::int64_t>(0);
    out.put<std::uint32_t>(1);
    out.string("Menu");
    out.put<std::uint32_t>(1);
    out.string("Start");
    out.put<std::uint32_t>(0x10);
    out.put<std::int32_t>(0);
    out.patch<std::int64_t>(offsetAt, static_cast<std::int64_t>(out.bytes.size()));
    out.put<std::int32_t>(1);
    writeStrings(out);
}

}

TEST(optimizedLocresReadsStringArray) {
    Writer out;
    writeOptimizedLocres(out, 3, false);
    FMemoryReaderView reader(out.bytes);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("Game.locres")));
    CHECK(resource.GetEntries().Num() == 2);
    const auto* start = findEntry(resource, "Menu", "Start");
    const auto* quit = findEntry(resource, "Menu", "Quit");
    REQUIRE(start != nullptr && quit != nullptr);
    CHECK(start->LocalizedString.ToUtf8() == "Play");
    CHECK(quit->LocalizedString.ToUtf8() == "Leave");
    CHECK(quit->SourceStringHash == 0x3334);
}

TEST(compactLocresStringsHaveNoRefCount) {
    Writer out;
    writeCompactLocres(out, [](Writer& w) { w.string("Play"); });
    FMemoryReaderView reader(out.bytes);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("Game.locres")));
    const auto* entry = findEntry(resource, "Menu", "Start");
    REQUIRE(entry != nullptr);
    CHECK(entry->LocalizedString.ToUtf8() == "Play");
}

TEST(locresRejectsNewerVersion) {
    Writer out;
    writeOptimizedLocres(out, 4, true);
    FMemoryReaderView reader(out.bytes);
    rt::FTextLocalizationResource resource;
    CHECK(!resource.LoadFromArchive(reader, rt::FString("Game.locres")));
    CHECK(reader.IsError());
}

TEST(stellarBladeLocresSkipsEntryExtra) {
    Writer out;
    writeOptimizedLocres(out, 4, true);
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_StellarBlade);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("Game.locres")));
    const auto* quit = findEntry(resource, "Menu", "Quit");
    REQUIRE(quit != nullptr);
    CHECK(quit->LocalizedString.ToUtf8() == "Leave");
}

TEST(honorOfKingsWorldLocresReadsTables) {
    Writer out;
    writeLocresHeader(out, 4);
    out.put<std::int32_t>(2);
    out.put<std::uint32_t>(1);
    out.put<std::uint32_t>(2);
    out.put<std::int32_t>(1);
    out.string("Table");
    out.put<std::int32_t>(1);
    out.put<std::int32_t>(0);
    out.put<std::uint32_t>(0x10);
    out.string("Hero");
    out.put<std::uint32_t>(0x20);
    out.string("Champion");
    out.string("Extra");
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_HonorofKingsWorld);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("Game.locres")));
    const auto* entry = findEntry(resource, "", "Hero");
    REQUIRE(entry != nullptr);
    CHECK(entry->LocalizedString.ToUtf8() == "Champion");
    CHECK(entry->SourceStringHash == 0x20);
    CHECK(reader.AtEnd());
}

TEST(codeVein2LocresDecryptsStrings) {
    Writer out;
    writeCompactLocres(out, [](Writer& w) {
        writeCodeVein2String(w, "Play", ue5parse::CodeVein2StringKey::Locres);
        w.put<std::int32_t>(1);
    });
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_CodeVein2);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("CodeVein2/Content/Localization/Game/en/Game.locres")));
    const auto* entry = findEntry(resource, "Menu", "Start");
    REQUIRE(entry != nullptr);
    CHECK(entry->LocalizedString.ToUtf8() == "Play");
    CHECK(reader.Tell() < static_cast<std::int64_t>(out.bytes.size()));
}

TEST(embersOfTheUncrownedLocresDecryptsStrings) {
    constexpr std::string_view key = "BM8c/12uWU7Z78MbVza5A6MrCeoTIaS6nIbXjFQNlrs=";
    std::vector<std::uint8_t> stored = {'O', 0, 'k', 0};
    for (std::size_t i = 0; i < stored.size(); i++)
        stored[i] ^= static_cast<std::uint8_t>(key[i % key.size()]);
    Writer out;
    writeCompactLocres(out, [&stored](Writer& w) {
        w.string(encodeBase64(stored) + "ENCRYPTED");
        w.put<std::int32_t>(1);
    });
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_EmbersofTheUncrowned);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("Game.locres")));
    const auto* entry = findEntry(resource, "Menu", "Start");
    REQUIRE(entry != nullptr);
    CHECK(entry->LocalizedString.ToUtf8() == "Ok");
}

namespace {

void writeNevernessToEvernessLocres(Writer& out, std::int32_t version, std::string_view stored) {
    writeLocresHeader(out, 1);
    out.put<std::int32_t>(version);
    const std::size_t offsetAt = out.bytes.size();
    out.put<std::int64_t>(0);
    out.put<std::uint32_t>(1);
    out.string("Menu");
    out.put<std::uint32_t>(1);
    out.string("Start");
    out.put<std::uint32_t>(0x10);
    out.put<std::int32_t>(0);
    out.patch<std::int64_t>(offsetAt, static_cast<std::int64_t>(out.bytes.size()));
    out.put<std::int32_t>(1);
    out.string(stored);
    out.put<std::int32_t>(1);
}

}

TEST(nevernessToEvernessLocresReadsPlainStrings) {
    Writer out;
    writeNevernessToEvernessLocres(out, 9000, "Play");
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_NevernessToEverness);
    rt::FTextLocalizationResource resource;
    REQUIRE(resource.LoadFromArchive(reader, rt::FString("HT/Content/Localization/Game/en/Game.locres")));
    const auto* entry = findEntry(resource, "Menu", "Start");
    REQUIRE(entry != nullptr);
    CHECK(entry->LocalizedString.ToUtf8() == "Play");
}

TEST(nevernessToEvernessLocresRejectsPartialBlock) {
    Writer out;
    writeNevernessToEvernessLocres(out, 10000, "AAAA");
    FMemoryReaderView reader(out.bytes);
    reader.SetGame(EGame::GAME_NevernessToEverness);
    rt::FTextLocalizationResource resource;
    CHECK(!resource.LoadFromArchive(reader, rt::FString("HT/Content/Localization/Game/en/Game.locres")));
}

namespace {

class NameArchive : public FMemoryReaderView {
public:
    using FMemoryReaderView::FMemoryReaderView;
    using FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = rt::FName(Text.ToUtf8());
        return *this;
    }
};

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

void writeStringTableStart(Writer& out) {
    out.put<std::uint16_t>(0x0100);
    out.put<std::uint32_t>(0);
    out.string("Table");
    out.put<std::int32_t>(1);
    out.string("Key");
}

std::optional<std::string> readStringTable(EGame game, const std::vector<std::byte>& bytes, bool& atEnd) {
    ue5parse::FileProvider provider(emptyGame(game));
    rt::FRuntime runtime(provider);
    NameArchive reader(bytes);
    reader.SetGame(game);
    rt::UStringTable* table = rt::NewObject<rt::UStringTable>(rt::GetTransientPackage());
    table->Serialize(reader);
    atEnd = reader.AtEnd();
    rt::FString source;
    if (reader.IsError() || !table->GetSourceString(rt::FString("Key"), source))
        return std::nullopt;
    return source.ToUtf8();
}

}

TEST(codeVein2StringTableDecryptsSources) {
    Writer out;
    writeStringTableStart(out);
    writeCodeVein2String(out, "Value", ue5parse::CodeVein2StringKey::StringTable);
    out.put<std::int32_t>(0);
    bool atEnd = false;
    CHECK(readStringTable(EGame::GAME_CodeVein2, out.bytes, atEnd) == "Value");
    CHECK(atEnd);
}

TEST(marvelRivalsStringTableSkipsSecondString) {
    Writer out;
    writeStringTableStart(out);
    out.string("Value");
    out.string("Skipped");
    out.put<std::int32_t>(0);
    bool atEnd = false;
    CHECK(readStringTable(EGame::GAME_MarvelRivals, out.bytes, atEnd) == "Value");
    CHECK(atEnd);
}

TEST(lostRecordsStringTableSkipsCountedStrings) {
    Writer out;
    writeStringTableStart(out);
    out.string("Value");
    out.string("Skipped");
    out.string("2");
    out.string("First");
    out.string("Second");
    out.put<std::int32_t>(0);
    bool atEnd = false;
    CHECK(readStringTable(EGame::GAME_LostRecordsBloomAndRage, out.bytes, atEnd) == "Value");
    CHECK(atEnd);
}

TEST(wildgateStringTableHasNoMetaData) {
    Writer out;
    writeStringTableStart(out);
    out.string("Value");
    bool atEnd = false;
    CHECK(readStringTable(EGame::GAME_Wildgate, out.bytes, atEnd) == "Value");
    CHECK(atEnd);
}
