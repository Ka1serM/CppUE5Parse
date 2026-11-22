#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::runtime::FMemoryReaderView;
using ue5parse::runtime::FText;

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
