#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "CppUE5Parse/Core/NameHash.h"

namespace ue5parse {

std::uint64_t hashNameRuntime(std::string_view text);
std::uint64_t cityHash64WithSeed(std::string_view text, std::uint64_t seed);

constexpr std::uint64_t hashName(std::string_view text) {
    if (text.size() > 1024)
        text = text.substr(0, 1024);
    if consteval {
        return namehash::cityHash64Lower(text);
    }
    return hashNameRuntime(text);
}

class FName {
public:
    constexpr FName() = default;
    constexpr FName(std::string_view fullText) {
        std::size_t end = fullText.size();
        std::size_t digits = 0;
        while (digits < end && fullText[end - 1 - digits] >= '0' && fullText[end - 1 - digits] <= '9')
            digits++;
        bool hasSuffix = digits > 0 && digits < end && fullText[end - 1 - digits] == '_' && digits <= 9 && (digits == 1 || fullText[end - digits] != '0');
        if (hasSuffix) {
            std::uint32_t value = 0;
            for (std::size_t i = end - digits; i < end; i++)
                value = value * 10 + static_cast<std::uint32_t>(fullText[i] - '0');
            text = fullText.substr(0, end - digits - 1);
            number = value + 1;
        } else {
            text = fullText;
        }
        hash = hashName(text);
    }
    constexpr FName(const char* text) : FName(std::string_view(text)) {}
    constexpr FName(std::string_view text, std::uint64_t hash, std::uint32_t number) : text(text), hash(hash), number(number) {}

    constexpr std::string_view getText() const { return text; }
    constexpr std::uint64_t getHash() const { return hash; }
    constexpr std::uint32_t getNumber() const { return number; }
    constexpr bool isNone() const { return hash == noneHash && number == 0; }
    std::string toString() const;
    void appendTo(std::string& out) const;

    constexpr bool operator==(const FName& other) const { return hash == other.hash && number == other.number; }

    static constexpr std::uint64_t noneHash = 0x75370f5ba6352dc2ull;

private:
    std::string_view text = "None";
    std::uint64_t hash = noneHash;
    std::uint32_t number = 0;
};

struct FNameHasher {
    std::size_t operator()(const FName& name) const { return name.getHash() ^ (std::uint64_t(name.getNumber()) << 47); }
};

}
