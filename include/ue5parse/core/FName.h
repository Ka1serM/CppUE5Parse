#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "ue5parse/core/NameHash.h"

namespace ue5parse {

// The run-time path of hashName: the library's CityHash64.
std::uint64_t hashNameRuntime(std::string_view text);

// The CityHash64 of the lowercase bytes of a name, as UE writes it into the Zen name batch. UE limits names to 1024
// characters (NAME_SIZE). Computed at compile time for constants.
constexpr std::uint64_t hashName(std::string_view text) {
    if (text.size() > 1024) text = text.substr(0, 1024);
    if consteval {
        return namehash::cityHash64Lower(text);
    }
    return hashNameRuntime(text);
}

// A UE name: its text without the number suffix, the hash of that text, and the number. Compare and hash in O(1),
// by the hash and the number only.
//
// The text is a view. Names from packages and mappings point into data that their FileProvider owns. A name made from
// a temporary string is good for lookups only: do not keep its text.
//
// A name from a literal hashes the literal on each call. In hot code use a constant, which is hashed at compile time:
//     static constexpr FName NotifyStateClass{"NotifyStateClass"};
class FName {
public:
    constexpr FName() = default;
    // Implicit, so that "Intensity" works as an argument. Splits a number suffix as UE does: "Mesh_3" is "Mesh", 4.
    constexpr FName(std::string_view fullText) {
        // UE's FName number rule: a suffix "_<digits>" without a leading zero is the number + 1.
        std::size_t end = fullText.size();
        std::size_t digits = 0;
        while (digits < end && fullText[end - 1 - digits] >= '0' && fullText[end - 1 - digits] <= '9') digits++;
        bool hasSuffix = digits > 0 && digits < end && fullText[end - 1 - digits] == '_' && digits <= 9 &&
                         (digits == 1 || fullText[end - digits] != '0');
        if (hasSuffix) {
            std::uint32_t value = 0;
            for (std::size_t i = end - digits; i < end; i++) value = value * 10 + static_cast<std::uint32_t>(fullText[i] - '0');
            text = fullText.substr(0, end - digits - 1);
            number = value + 1;
        } else {
            text = fullText;
        }
        hash = hashName(text);
    }
    constexpr FName(const char* text) : FName(std::string_view(text)) {}
    // Text that is already split, with a known hash: from a name batch or a mappings file.
    constexpr FName(std::string_view text, std::uint64_t hash, std::uint32_t number) : text(text), hash(hash), number(number) {}

    constexpr std::string_view getText() const { return text; }
    constexpr std::uint64_t getHash() const { return hash; }
    constexpr std::uint32_t getNumber() const { return number; }
    constexpr bool isNone() const { return hash == noneHash && number == 0; }
    // The text with the number suffix, as UE and CUE4Parse show it: "Mesh_3".
    std::string toString() const;
    void appendTo(std::string& out) const;

    constexpr bool operator==(const FName& other) const { return hash == other.hash && number == other.number; }

    // hashName("None"), written out so that it does not depend on static initialization order.
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
