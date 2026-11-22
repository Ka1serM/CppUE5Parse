#include "CppUE5Parse/Crypto/Blake3.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <vector>

namespace ue5parse {

namespace {

using Words = std::array<std::uint32_t, 8>;
using Block = std::array<std::uint32_t, 16>;

constexpr Words initialVector = {0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};
constexpr std::array<std::size_t, 16> permutation = {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8};
constexpr std::uint32_t chunkStart = 1;
constexpr std::uint32_t chunkEnd = 2;
constexpr std::uint32_t parent = 4;
constexpr std::uint32_t root = 8;
constexpr std::size_t blockLength = 64;
constexpr std::size_t chunkLength = 1024;

void mix(Block& state, std::size_t a, std::size_t b, std::size_t c, std::size_t d, std::uint32_t x, std::uint32_t y) {
    state[a] += state[b] + x;
    state[d] = std::rotr(state[d] ^ state[a], 16);
    state[c] += state[d];
    state[b] = std::rotr(state[b] ^ state[c], 12);
    state[a] += state[b] + y;
    state[d] = std::rotr(state[d] ^ state[a], 8);
    state[c] += state[d];
    state[b] = std::rotr(state[b] ^ state[c], 7);
}

Words compress(const Words& chaining, Block message, std::uint64_t counter, std::uint32_t length, std::uint32_t flags) {
    Block state = {chaining[0],
                   chaining[1],
                   chaining[2],
                   chaining[3],
                   chaining[4],
                   chaining[5],
                   chaining[6],
                   chaining[7],
                   initialVector[0],
                   initialVector[1],
                   initialVector[2],
                   initialVector[3],
                   static_cast<std::uint32_t>(counter),
                   static_cast<std::uint32_t>(counter >> 32),
                   length,
                   flags};
    for (int round = 0; round < 7; round++) {
        mix(state, 0, 4, 8, 12, message[0], message[1]);
        mix(state, 1, 5, 9, 13, message[2], message[3]);
        mix(state, 2, 6, 10, 14, message[4], message[5]);
        mix(state, 3, 7, 11, 15, message[6], message[7]);
        mix(state, 0, 5, 10, 15, message[8], message[9]);
        mix(state, 1, 6, 11, 12, message[10], message[11]);
        mix(state, 2, 7, 8, 13, message[12], message[13]);
        mix(state, 3, 4, 9, 14, message[14], message[15]);
        Block permuted{};
        for (std::size_t i = 0; i < 16; i++)
            permuted[i] = message[permutation[i]];
        message = permuted;
    }
    Words output{};
    for (std::size_t i = 0; i < 8; i++)
        output[i] = state[i] ^ state[i + 8];
    return output;
}

Block loadBlock(std::span<const std::byte> bytes) {
    std::array<std::byte, blockLength> padded{};
    std::memcpy(padded.data(), bytes.data(), bytes.size());
    Block block{};
    for (std::size_t i = 0; i < 16; i++)
        block[i] = static_cast<std::uint32_t>(padded[i * 4]) | (static_cast<std::uint32_t>(padded[i * 4 + 1]) << 8)
                   | (static_cast<std::uint32_t>(padded[i * 4 + 2]) << 16) | (static_cast<std::uint32_t>(padded[i * 4 + 3]) << 24);
    return block;
}

Block joinWords(const Words& left, const Words& right) {
    Block block{};
    std::memcpy(block.data(), left.data(), sizeof(left));
    std::memcpy(block.data() + 8, right.data(), sizeof(right));
    return block;
}

Words hashChunk(std::span<const std::byte> chunk, std::uint64_t counter, std::uint32_t extraFlags) {
    Words chaining = initialVector;
    std::size_t blocks = chunk.empty() ? 1 : (chunk.size() + blockLength - 1) / blockLength;
    for (std::size_t i = 0; i < blocks; i++) {
        std::span<const std::byte> bytes = chunk.subspan(i * blockLength, std::min(blockLength, chunk.size() - i * blockLength));
        std::uint32_t flags = (i == 0 ? chunkStart : 0) | (i + 1 == blocks ? chunkEnd | extraFlags : 0);
        chaining = compress(chaining, loadBlock(bytes), counter, static_cast<std::uint32_t>(bytes.size()), flags);
    }
    return chaining;
}

}

std::array<std::uint8_t, 32> hashBlake3(std::span<const std::byte> input) {
    std::size_t chunks = input.empty() ? 1 : (input.size() + chunkLength - 1) / chunkLength;
    Words output{};
    if (chunks == 1) {
        output = hashChunk(input, 0, root);
    } else {
        std::vector<Words> stack;
        for (std::size_t i = 0; i + 1 < chunks; i++) {
            Words chaining = hashChunk(input.subspan(i * chunkLength, chunkLength), i, 0);
            for (std::size_t total = i + 1; (total & 1) == 0; total >>= 1) {
                chaining = compress(initialVector, joinWords(stack.back(), chaining), 0, blockLength, parent);
                stack.pop_back();
            }
            stack.push_back(chaining);
        }
        output = hashChunk(input.subspan((chunks - 1) * chunkLength), chunks - 1, 0);
        while (!stack.empty()) {
            std::uint32_t flags = stack.size() == 1 ? parent | root : parent;
            output = compress(initialVector, joinWords(stack.back(), output), 0, blockLength, flags);
            stack.pop_back();
        }
    }
    std::array<std::uint8_t, 32> digest{};
    for (std::size_t i = 0; i < 32; i++)
        digest[i] = static_cast<std::uint8_t>(output[i / 4] >> ((i % 4) * 8));
    return digest;
}

}
