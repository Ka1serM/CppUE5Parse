#include "CppUE5Parse/GameTypes/Century/CenturyDecrypt.h"

#include <array>
#include <cstdint>

namespace ue5parse {

namespace {

//! The distances between the XORed bytes, reversed by Spiritovod.
constexpr std::array<std::uint8_t, 470> centuryDistances = {
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7, 6, 4, 5, 9, 5, 3, 8, 7, 5, 3,
    7, 4, 6, 2, 8, 3, 7, 2, 7, 5, 6, 1, 10, 3, 2, 4, 3, 5, 3, 3, 4, 4, 4, 3,
    3, 6, 3, 2, 6, 4, 4, 5, 5, 8, 5, 4, 6, 7,
};

}

void decryptCenturyPwc(std::span<std::byte> block) {
    std::int64_t size = static_cast<std::int64_t>(block.size());
    if (size <= 5)
        return;
    std::size_t count = 0;
    std::int64_t from = size - 5;
    std::int64_t to = size - 8;
    std::byte key = block[static_cast<std::size_t>(from)];
    while (to >= 0 && count + 1 < centuryDistances.size()) {
        block[static_cast<std::size_t>(to)] ^= key;
        from = to - centuryDistances[count];
        if (from <= 0)
            break;
        key ^= block[static_cast<std::size_t>(from)];
        to = from - centuryDistances[count + 1];
        count += 2;
    }
    if (from <= 0 || to <= 0)
        return;
    block[static_cast<std::size_t>(to)] ^= key;
    constexpr std::int64_t tail[][2] = {{6, 10}, {15, 24}, {29, 32}, {40, 47}};
    for (const auto& [source, target] : tail) {
        if (to < target)
            return;
        key ^= block[static_cast<std::size_t>(to - source)];
        block[static_cast<std::size_t>(to - target)] ^= key;
    }
}

}
