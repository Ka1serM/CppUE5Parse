#include "CppUE5Parse/GameTypes/MarvelRivals/MarvelRivalsAes.h"

#include <algorithm>

namespace ue5parse {

namespace {

void reverseWords(std::span<std::byte> data) {
    for (std::size_t offset = 0; offset + 4 <= data.size(); offset += 4)
        std::ranges::reverse(data.subspan(offset, 4));
}

}

//! The reference rijndael.c code in the game loads and stores each 32-bit word in little-endian order.
void decryptMarvelRivals(std::span<std::byte> data, const AesKey& key) {
    AesKey swapped = key;
    reverseWords(swapped);
    reverseWords(data);
    decryptAesEcb(swapped, data);
    reverseWords(data);
}

}
