#include "CppUE5Parse/Crypto/Sm4.h"

#include <bit>

namespace ue5parse::Sm4 {

namespace {

std::uint32_t loadBigEndian(std::span<const std::byte> bytes) {
    return (static_cast<std::uint32_t>(bytes[0]) << 24) | (static_cast<std::uint32_t>(bytes[1]) << 16) | (static_cast<std::uint32_t>(bytes[2]) << 8)
           | static_cast<std::uint32_t>(bytes[3]);
}

void storeBigEndian(std::span<std::byte> bytes, std::uint32_t value) {
    bytes[0] = static_cast<std::byte>(value >> 24);
    bytes[1] = static_cast<std::byte>(value >> 16);
    bytes[2] = static_cast<std::byte>(value >> 8);
    bytes[3] = static_cast<std::byte>(value);
}

std::uint32_t substitute(std::uint32_t value, const Sbox& sbox) {
    return static_cast<std::uint32_t>(sbox[value & 0xFF]) | (static_cast<std::uint32_t>(sbox[(value >> 8) & 0xFF]) << 8)
           | (static_cast<std::uint32_t>(sbox[(value >> 16) & 0xFF]) << 16) | (static_cast<std::uint32_t>(sbox[value >> 24]) << 24);
}

}

RoundKeys expandKey(std::span<const std::byte, 16> key, const Sbox& sbox, const std::array<std::uint32_t, 4>& familyKey,
                    std::span<const std::uint32_t, 32> constantKey) {
    std::array<std::uint32_t, 4> words{};
    for (std::size_t i = 0; i < 4; i++)
        words[i] = loadBigEndian(key.subspan(i * 4, 4)) ^ familyKey[i];
    RoundKeys roundKeys{};
    for (std::size_t i = 0; i < 32; i++) {
        std::uint32_t entry = substitute(constantKey[i] ^ words[(i + 1) % 4] ^ words[(i + 2) % 4] ^ words[(i + 3) % 4], sbox);
        std::uint32_t value = entry ^ words[i % 4] ^ std::rotr(entry, 9) ^ std::rotl(entry, 13);
        words[i % 4] = value;
        roundKeys[i] = value;
    }
    return roundKeys;
}

void decrypt(std::span<std::byte> data, const RoundKeys& roundKeys, const Sbox& sbox) {
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        std::span<std::byte> block = data.subspan(offset, 16);
        std::array<std::uint32_t, 4> words{};
        for (std::size_t i = 0; i < 4; i++)
            words[i] = loadBigEndian(block.subspan(i * 4, 4));
        for (std::size_t i = 0; i < 32; i++) {
            std::uint32_t entry = substitute(roundKeys[31 - i] ^ words[(i + 1) % 4] ^ words[(i + 2) % 4] ^ words[(i + 3) % 4], sbox);
            words[i % 4] ^= entry ^ std::rotl(entry, 2) ^ std::rotr(entry, 8) ^ std::rotl(entry, 10) ^ std::rotr(entry, 14);
        }
        for (std::size_t i = 0; i < 4; i++)
            storeBigEndian(block.subspan(i * 4, 4), words[3 - i]);
    }
}

}
