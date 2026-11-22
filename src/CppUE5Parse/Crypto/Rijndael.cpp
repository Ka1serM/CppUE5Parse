#include "CppUE5Parse/Crypto/Rijndael.h"

#include <cstring>

namespace ue5parse::Rijndael {

namespace {

std::uint32_t loadBigEndian(std::span<const std::byte> bytes) {
    return (static_cast<std::uint32_t>(bytes[0]) << 24) | (static_cast<std::uint32_t>(bytes[1]) << 16) | (static_cast<std::uint32_t>(bytes[2]) << 8)
           | static_cast<std::uint32_t>(bytes[3]);
}

std::uint32_t subWord(std::uint32_t word) {
    return (te4[(word >> 24) & 0xFF] & 0xFF000000) ^ (te4[(word >> 16) & 0xFF] & 0x00FF0000) ^ (te4[(word >> 8) & 0xFF] & 0x0000FF00)
           ^ (te4[word & 0xFF] & 0x000000FF);
}

std::uint32_t rotateWord(std::uint32_t word) {
    return (word << 8) | (word >> 24);
}

std::uint32_t inverseMixWord(std::uint32_t word) {
    return td0[te4[(word >> 24) & 0xFF] & 0xFF] ^ td1[te4[(word >> 16) & 0xFF] & 0xFF] ^ td2[te4[(word >> 8) & 0xFF] & 0xFF] ^ td3[te4[word & 0xFF] & 0xFF];
}

std::array<std::uint32_t, 60> expandWords256(std::span<const std::byte, 32> key, std::uint32_t secondWordMask) {
    std::array<std::uint32_t, 60> words{};
    for (std::size_t i = 0; i < 8; i++)
        words[i] = loadBigEndian(key.subspan(i * 4, 4));
    words[1] ^= secondWordMask;
    std::uint32_t roundConstant = 0x01000000;
    for (std::size_t i = 8; i < 60; i++) {
        std::uint32_t temp = words[i - 1];
        if (i % 8 == 0) {
            temp = subWord(rotateWord(temp)) ^ roundConstant;
            roundConstant = static_cast<std::uint32_t>(xtime(static_cast<std::uint8_t>(roundConstant >> 24))) << 24;
        } else if (i % 8 == 4) {
            temp = subWord(temp);
        }
        words[i] = words[i - 8] ^ temp;
    }
    return words;
}

Block decryptRound(const Block& state, const Block& roundKey, bool last) {
    Block shifted{};
    for (std::size_t column = 0; column < 4; column++)
        for (std::size_t row = 0; row < 4; row++)
            shifted[column * 4 + row] = inverseSbox[state[((column + 4 - row) % 4) * 4 + row]];
    Block mixed = last ? shifted : inverseMixColumns(shifted);
    for (std::size_t i = 0; i < 16; i++)
        mixed[i] ^= roundKey[i];
    return mixed;
}

}

RoundKeys setupDecrypt256(std::span<const std::byte, 32> key, std::uint32_t secondWordMask) {
    std::array<std::uint32_t, 60> words = expandWords256(key, secondWordMask);
    RoundKeys keys{};
    for (std::size_t round = 0; round <= 14; round++)
        for (std::size_t i = 0; i < 4; i++)
            keys[round * 4 + i] = words[(14 - round) * 4 + i];
    for (std::size_t i = 4; i < 56; i++)
        keys[i] = inverseMixWord(keys[i]);
    return keys;
}

std::array<Block, 15> expandKey256(std::span<const std::byte, 32> key) {
    std::array<std::uint32_t, 60> words = expandWords256(key, 0);
    std::array<Block, 15> blocks{};
    for (std::size_t i = 0; i < 60; i++)
        for (std::size_t j = 0; j < 4; j++)
            blocks[i / 4][(i % 4) * 4 + j] = static_cast<std::uint8_t>(words[i] >> (24 - j * 8));
    return blocks;
}

Block inverseMixColumns(const Block& block) {
    Block mixed{};
    for (std::size_t column = 0; column < 16; column += 4) {
        std::uint8_t a = block[column];
        std::uint8_t b = block[column + 1];
        std::uint8_t c = block[column + 2];
        std::uint8_t d = block[column + 3];
        mixed[column] = multiply(a, 14) ^ multiply(b, 11) ^ multiply(c, 13) ^ multiply(d, 9);
        mixed[column + 1] = multiply(a, 9) ^ multiply(b, 14) ^ multiply(c, 11) ^ multiply(d, 13);
        mixed[column + 2] = multiply(a, 13) ^ multiply(b, 9) ^ multiply(c, 14) ^ multiply(d, 11);
        mixed[column + 3] = multiply(a, 11) ^ multiply(b, 13) ^ multiply(c, 9) ^ multiply(d, 14);
    }
    return mixed;
}

void decryptWithRoundKeys(std::span<std::byte> data, std::span<const Block> roundKeys) {
    std::size_t last = roundKeys.size() - 1;
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        Block state{};
        std::memcpy(state.data(), data.data() + offset, 16);
        for (std::size_t i = 0; i < 16; i++)
            state[i] ^= roundKeys[0][i];
        for (std::size_t round = 1; round < last; round++)
            state = decryptRound(state, roundKeys[round], false);
        state = decryptRound(state, roundKeys[last], true);
        std::memcpy(data.data() + offset, state.data(), 16);
    }
}

}
