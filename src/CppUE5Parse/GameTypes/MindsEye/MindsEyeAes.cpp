#include "CppUE5Parse/GameTypes/MindsEye/MindsEyeAes.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace ue5parse {

namespace {

void reverseWords(std::span<std::byte> data) {
    for (std::size_t offset = 0; offset + 4 <= data.size(); offset += 4)
        std::ranges::reverse(data.subspan(offset, 4));
}

constexpr std::array<std::uint8_t, 256> makeLookupTable() {
    std::array<std::uint8_t, 256> table{};
    for (std::uint32_t n = 0; n < 256; n++) {
        std::uint32_t a = n & 0xCB;
        std::uint32_t b = n & 0x34;
        std::uint32_t c = n | 0xCB;
        std::uint32_t d = n | 0x34;
        std::uint32_t e = (a * 0xFD) & 0xFF;
        std::uint32_t f = (b * 0xFD) & 0xFF;
        table[n] = static_cast<std::uint8_t>(((~b + d) * 2 + ~(~e | f) + (~e & f) + (e & f) + c * 3) * 2 - 0x59 + ~(e ^ f));
    }
    return table;
}

constexpr std::array<std::uint8_t, 256> lookupTable = makeLookupTable();

void unmaskIndex(std::span<std::byte> data) {
    std::uint32_t seed = 0x23212002;
    for (std::byte& value : data) {
        std::uint32_t n = static_cast<std::uint8_t>(value);
        std::uint32_t a = (n | seed) & 0xFF;
        std::uint32_t b = (~n | seed) & 0xFF;
        std::uint32_t c = (n & seed) & 0xFF;
        std::uint32_t d = (~n & seed) & 0xFF;
        std::uint32_t e = (n ^ seed) & 0xFF;
        std::uint32_t f = (a + seed) & 0xFF;
        std::uint32_t g = (d * 2 + c) & 0xFF;
        std::uint32_t r = ((((~a + 1 + c) * 4 + e * 3 - b + d) * 2 + ((g - seed) * 2 - e) * 9) * ~b) & 0xFF;
        r += (((f - (d + c) * 2) * 2 - d * 3 + e) * 3 - 1) * e;
        r += (g * 9 - a * 12 + 2) * 2 * d;
        r += (a * 6 - d * 9 - 1) * 2 * seed;
        r += ((g - f) * 6 + 1) * 2 * c;
        r &= 0xFF;
        value = static_cast<std::byte>(r);
        seed = ~(~r | seed) * 3 + ~(r ^ seed) + (r | seed) + (r & seed) + (~r & seed) - r * 2 + 1;
    }
}

}

void decryptMindsEye(std::span<std::byte> data, const AesKey& key, bool isIndex) {
    AesKey swapped = key;
    reverseWords(swapped);
    reverseWords(data);
    decryptAesEcb(swapped, data);
    reverseWords(data);
    if (isIndex) {
        unmaskIndex(data);
        return;
    }
    for (std::byte& value : data)
        value = static_cast<std::byte>(lookupTable[static_cast<std::uint8_t>(value)]);
}

}
