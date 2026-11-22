#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ue5parse::Rijndael {

using Table = std::array<std::uint32_t, 256>;
using Block = std::array<std::uint8_t, 16>;
using RoundKeys = std::array<std::uint32_t, 60>;

constexpr std::uint8_t xtime(std::uint8_t value) {
    return static_cast<std::uint8_t>((value << 1) ^ ((value & 0x80) != 0 ? 0x1B : 0));
}

constexpr std::uint8_t multiply(std::uint8_t a, std::uint8_t b) {
    std::uint8_t result = 0;
    for (; b != 0; b >>= 1, a = xtime(a))
        if ((b & 1) != 0)
            result ^= a;
    return result;
}

constexpr std::uint8_t rotateLeft(std::uint8_t value, int shift) {
    return static_cast<std::uint8_t>((value << shift) | (value >> (8 - shift)));
}

constexpr std::array<std::uint8_t, 256> makeSbox() {
    std::array<std::uint8_t, 256> sbox{};
    sbox[0] = 0x63;
    std::uint8_t power = 1;
    std::uint8_t inverse = 1;
    do {
        power ^= xtime(power);
        inverse ^= static_cast<std::uint8_t>(inverse << 1);
        inverse ^= static_cast<std::uint8_t>(inverse << 2);
        inverse ^= static_cast<std::uint8_t>(inverse << 4);
        if ((inverse & 0x80) != 0)
            inverse ^= 0x09;
        std::uint8_t affine = inverse ^ rotateLeft(inverse, 1) ^ rotateLeft(inverse, 2) ^ rotateLeft(inverse, 3) ^ rotateLeft(inverse, 4);
        sbox[power] = static_cast<std::uint8_t>(affine ^ 0x63);
    } while (power != 1);
    return sbox;
}

inline constexpr std::array<std::uint8_t, 256> sbox = makeSbox();

constexpr std::array<std::uint8_t, 256> makeInverseSbox() {
    std::array<std::uint8_t, 256> inverse{};
    for (int i = 0; i < 256; i++)
        inverse[sbox[i]] = static_cast<std::uint8_t>(i);
    return inverse;
}

inline constexpr std::array<std::uint8_t, 256> inverseSbox = makeInverseSbox();

constexpr std::uint32_t rotateRight(std::uint32_t value, int shift) {
    return (value >> shift) | (value << (32 - shift));
}

constexpr Table makeDecryptTable(int rotation) {
    Table table{};
    for (int i = 0; i < 256; i++) {
        std::uint8_t s = inverseSbox[i];
        std::uint32_t word = (static_cast<std::uint32_t>(multiply(s, 14)) << 24) | (static_cast<std::uint32_t>(multiply(s, 9)) << 16)
                             | (static_cast<std::uint32_t>(multiply(s, 13)) << 8) | multiply(s, 11);
        table[i] = rotation == 0 ? word : rotateRight(word, rotation);
    }
    return table;
}

constexpr Table makeRepeatedTable(const std::array<std::uint8_t, 256>& bytes) {
    Table table{};
    for (int i = 0; i < 256; i++)
        table[i] = bytes[i] * 0x01010101u;
    return table;
}

inline constexpr Table te4 = makeRepeatedTable(sbox);
inline constexpr Table td0 = makeDecryptTable(0);
inline constexpr Table td1 = makeDecryptTable(8);
inline constexpr Table td2 = makeDecryptTable(16);
inline constexpr Table td3 = makeDecryptTable(24);
inline constexpr Table td4 = makeRepeatedTable(inverseSbox);

//! The reference rijndael.c key schedule for a 256-bit key, with a mask XORed into its second word as Spectre Divide does.
RoundKeys setupDecrypt256(std::span<const std::byte, 32> key, std::uint32_t secondWordMask);

std::array<Block, 15> expandKey256(std::span<const std::byte, 32> key);
Block inverseMixColumns(const Block& block);

//! Runs the AES-NI decryption sequence: XOR the first round key, AESDEC with the middle keys and AESDECLAST with the last.
void decryptWithRoundKeys(std::span<std::byte> data, std::span<const Block> roundKeys);

}
