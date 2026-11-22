#include "CppUE5Parse/GameTypes/ArcRaiders/ArcRaidersAes.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>

namespace ue5parse {

namespace {

std::uint64_t loadUint64(std::span<const std::byte> bytes) {
    std::uint64_t value = 0;
    std::memcpy(&value, bytes.data(), 8);
    return value;
}

void unmaskIndex(std::span<std::byte> data, const AesKey& key) {
    std::array<std::uint64_t, 34> roundKeys{};
    std::array<std::uint64_t, 36> schedule{};
    std::span<const std::byte> keyBytes(key);
    roundKeys[0] = loadUint64(keyBytes.subspan(0, 8));
    schedule[0] = loadUint64(keyBytes.subspan(8, 8));
    schedule[1] = loadUint64(keyBytes.subspan(16, 8));
    schedule[2] = loadUint64(keyBytes.subspan(24, 8));
    for (std::uint64_t i = 0; i < 33; i++) {
        schedule[i + 3] = (roundKeys[i] + std::rotr(schedule[i], 8)) ^ i;
        roundKeys[i + 1] = std::rotl(roundKeys[i], 3) ^ schedule[i + 3];
    }
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        std::uint64_t right = loadUint64(data.subspan(offset, 8));
        std::uint64_t left = loadUint64(data.subspan(offset + 8, 8));
        for (std::size_t i = 34; i-- > 0;) {
            right = std::rotr(right ^ left, 3);
            left = std::rotl((left ^ roundKeys[i]) - right, 8);
        }
        std::memcpy(data.data() + offset, &right, 8);
        std::memcpy(data.data() + offset + 8, &left, 8);
    }
}

}

void decryptArcRaiders(std::span<std::byte> data, const AesKey& key, bool isIndex) {
    if (isIndex)
        unmaskIndex(data, key);
    decryptAesEcb(key, data);
}

}
