#include "CppUE5Parse/GameTypes/ProSpi/ProSpiDecrypt.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <vector>

namespace ue5parse {

namespace {

constexpr std::size_t blockSize = 64;
constexpr std::size_t mtStateSize = 312;
constexpr std::size_t mtMiddleWord = 156;
constexpr std::uint64_t mtMatrixA = 0xB5026F5AA96619E9;
constexpr std::uint64_t mtUpperMask = 0xFFFFFFFF80000000;
constexpr std::uint64_t mtLowerMask = 0x7FFFFFFF;

enum class KeyLayout : std::uint8_t { Full, First16Repeated, Second16Repeated, Swap16 };

enum class NonceLayout : std::uint8_t { Nonce64, Nonce96, Nonce64WithFirstWord, SalsaSeed012345, SalsaSeed234501, SalsaSeed345012 };

enum class CipherKind : std::uint8_t {
    ChaCha,
    ProSpiCustomPermuted,
    ProSpiCustomPermutedAlt,
    ProSpiSalsaRorPremixed,
    ProSpiCustomPermutedPremixed,
    ProSpiSalsaRor
};

struct CipherSpec {
    std::uint64_t descriptor;
    KeyLayout keyLayout;
    NonceLayout nonceLayout;
    std::uint32_t nonceOffset;
    std::uint32_t rounds;
    CipherKind kind = CipherKind::ChaCha;
    std::uint32_t premixRounds = 0;
};

using K = KeyLayout;
using N = NonceLayout;
using C = CipherKind;

constexpr std::array<CipherSpec, 31> cipherSpecs = {{
    {0x5E51B1FA6923008F, K::Full, N::Nonce96, 12, 18},
    {0xCE7E88D8DD84F42C, K::Full, N::Nonce64, 12, 12},
    {0xAC7044A9A2A16B70, K::First16Repeated, N::Nonce96, 12, 20},
    {0xDB8419469B3B9E4C, K::Full, N::Nonce64, 8, 10},
    {0x3B846FC8D3091AEE, K::Full, N::Nonce64, 8, 8},
    {0x185EB3B0ADCC5BF8, K::Full, N::Nonce64, 16, 10},
    {0xAFC4D1B9A88C5583, K::Full, N::Nonce96, 8, 10},
    {0x2CEB04D65DD121E1, K::Swap16, N::Nonce96, 0, 10},
    {0xE80C01FBC8BAE580, K::First16Repeated, N::Nonce64, 0, 10},
    {0xD88D7AFA8A718C14, K::Full, N::Nonce96, 0, 10},
    {0x18BD09EA1FF2E264, K::Full, N::Nonce64WithFirstWord, 16, 14},
    {0x5CFB14BDC4A27D3A, K::Full, N::Nonce64, 0, 10, C::ProSpiCustomPermuted},
    {0x78BD79F475FD1FD0, K::Swap16, N::Nonce64, 0, 6, C::ProSpiCustomPermuted},
    {0x3990293EF365A428, K::Full, N::Nonce64, 8, 6, C::ProSpiCustomPermuted},
    {0xCA2F4E8D213D2BEF, K::Full, N::Nonce64, 0, 8, C::ProSpiCustomPermuted},
    {0xE50C453CF956F6ED, K::Swap16, N::Nonce64, 0, 6, C::ProSpiCustomPermuted},
    {0xF55D9E838C8CC6B5, K::First16Repeated, N::Nonce64, 0, 7, C::ProSpiCustomPermuted},
    {0xB3341ED88A723037, K::Second16Repeated, N::Nonce64, 8, 6, C::ProSpiCustomPermuted},
    {0x0C256B87177991D4, K::First16Repeated, N::Nonce64, 8, 4, C::ProSpiCustomPermuted},
    {0x5583760930DE62E1, K::Full, N::Nonce64, 12, 10, C::ProSpiCustomPermuted},
    {0x3154B86A7EC79D9C, K::Full, N::Nonce64, 8, 5, C::ProSpiCustomPermuted},
    {0x848CA23594AF2BDE, K::Swap16, N::Nonce64, 8, 6, C::ProSpiCustomPermuted},
    {0x161320FC4AAD784F, K::Full, N::Nonce64, 16, 7, C::ProSpiCustomPermuted},
    {0xF3E438E3CCBA07E6, K::Swap16, N::Nonce64, 16, 5, C::ProSpiCustomPermuted},
    {0x02D03D38AFCCF46F, K::Full, N::Nonce64, 12, 6, C::ProSpiCustomPermutedAlt},
    {0xC97484323BD6ADBA, K::Full, N::SalsaSeed345012, 0, 8, C::ProSpiCustomPermutedPremixed, 8},
    {0xF35125988F54AF14, K::Swap16, N::Nonce64, 8, 8, C::ProSpiSalsaRor},
    {0x6EC058461FC7F09D, K::First16Repeated, N::Nonce64, 12, 4, C::ProSpiSalsaRor},
    {0x19018DB8D0B6F6C2, K::Full, N::SalsaSeed234501, 0, 4, C::ProSpiSalsaRorPremixed, 4},
    {0xDFBC689E3EB61740, K::Second16Repeated, N::SalsaSeed012345, 0, 5, C::ProSpiSalsaRorPremixed, 5},
    {0xF00101AAF0B9CB84, K::Full, N::SalsaSeed345012, 0, 6, C::ProSpiSalsaRorPremixed, 6},
}};

using State = std::array<std::uint32_t, 16>;
using Block = std::array<std::byte, blockSize>;

std::uint32_t readWord(std::span<const std::byte> bytes, std::size_t word) {
    std::uint32_t value;
    std::memcpy(&value, bytes.data() + word * 4, 4);
    return value;
}

void writeWords(const State& words, Block& block) {
    std::memcpy(block.data(), words.data(), blockSize);
}

std::uint32_t ror(std::uint32_t value, int shift) {
    return std::rotr(value, shift);
}

void generateSeedSequence(std::span<const std::uint32_t> seedInput, std::span<std::uint32_t> output) {
    std::ranges::fill(output, 0x8B8B8B8BU);
    const std::size_t n = output.size();
    const std::size_t s = seedInput.size();
    const std::size_t t = n >= 623 ? 11 : n >= 68 ? 7 : n >= 39 ? 5 : n >= 7 ? 3 : (n - 1) / 2;
    const std::size_t p = (n - t) / 2;
    const std::size_t q = p + t;
    const std::size_t m = std::max(s + 1, n);
    auto transform = [](std::uint32_t value) { return value ^ (value >> 27); };
    for (std::size_t k = 0; k < m; k++) {
        const std::uint32_t r1 = 1664525U * transform(output[k % n] ^ output[(k + p) % n] ^ output[(k + n - 1) % n]);
        output[(k + p) % n] += r1;
        const auto kMod = static_cast<std::uint32_t>(k % n);
        const std::uint32_t add = k == 0 ? static_cast<std::uint32_t>(s) : k <= s ? seedInput[k - 1] + kMod : kMod;
        const std::uint32_t r2 = r1 + add;
        output[(k + q) % n] += r2;
        output[k % n] = r2;
    }
    for (std::size_t k = m; k < m + n; k++) {
        const std::uint32_t r3 = 1566083941U * transform(output[k % n] + output[(k + p) % n] + output[(k + n - 1) % n]);
        output[(k + p) % n] ^= r3;
        const std::uint32_t r4 = r3 - static_cast<std::uint32_t>(k % n);
        output[(k + q) % n] ^= r4;
        output[k % n] = r4;
    }
}

struct MersenneTwister64 {
    std::array<std::uint64_t, mtStateSize> state{};
    std::size_t index = mtStateSize;

    void seed(std::uint64_t value) {
        state[0] = value;
        for (std::size_t i = 1; i < mtStateSize; i++)
            state[i] = 0x5851F42D4C957F2DULL * (state[i - 1] ^ (state[i - 1] >> 62)) + i;
        index = mtStateSize;
    }

    void seed(std::span<const std::uint32_t> seedInput) {
        std::array<std::uint32_t, mtStateSize * 2> generated{};
        generateSeedSequence(seedInput, generated);
        for (std::size_t i = 0; i < mtStateSize; i++)
            state[i] = generated[i * 2] | (static_cast<std::uint64_t>(generated[i * 2 + 1]) << 32);
        index = mtStateSize;
    }

    std::uint64_t next() {
        if (index >= mtStateSize) {
            for (std::size_t i = 0; i < mtStateSize; i++) {
                const std::uint64_t x = (state[i] & mtUpperMask) | (state[(i + 1) % mtStateSize] & mtLowerMask);
                std::uint64_t xA = x >> 1;
                if ((x & 1) != 0)
                    xA ^= mtMatrixA;
                state[i] = state[(i + mtMiddleWord) % mtStateSize] ^ xA;
            }
            index = 0;
        }
        std::uint64_t y = state[index++];
        y ^= (y >> 29) & 0x5555555555555555ULL;
        y ^= (y << 17) & 0x71D67FFFEDA60000ULL;
        y ^= (y << 37) & 0xFFF7EEE000000000ULL;
        y ^= y >> 43;
        return y;
    }
};

const CipherSpec* findSpec(std::span<const std::byte> trailer) {
    const std::uint64_t lookupKey = getProSpiDescriptorLookupKey(trailer);
    const auto* entry = std::ranges::lower_bound(proSpiDescriptorTable, lookupKey, {}, &ProSpiDescriptorEntry::lookupKey);
    if (entry == proSpiDescriptorTable.end() || entry->lookupKey != lookupKey)
        return nullptr;
    const auto* spec = std::ranges::find(cipherSpecs, entry->descriptor, &CipherSpec::descriptor);
    if (spec == cipherSpecs.end())
        return nullptr;
    return spec;
}

std::array<std::uint32_t, 8> readKey(const AesKey& key) {
    std::array<std::uint32_t, 8> words{};
    std::memcpy(words.data(), key.data(), key.size());
    return words;
}

void setConstants(State& state) {
    state[0] = 0x61707865;
    state[1] = 0x3120646E;
    state[2] = 0x79622D36;
    state[3] = 0x6B206574;
}

void setKeyHalves(State& state, const AesKey& key, KeyLayout layout) {
    const std::size_t firstHalf = layout == K::Swap16 || layout == K::Second16Repeated ? 16 : 0;
    std::size_t secondHalf = 16;
    if (layout == K::First16Repeated || layout == K::Swap16)
        secondHalf = 0;
    const std::span<const std::byte> bytes(key);
    for (std::size_t i = 0; i < 4; i++) {
        state[4 + i] = readWord(bytes.subspan(firstHalf), i);
        state[8 + i] = readWord(bytes.subspan(secondHalf), i);
    }
}

template <typename MakeBlock>
void xorStream(std::span<std::byte> payload, State& state, std::size_t counterLow, std::size_t counterHigh, MakeBlock makeBlock) {
    Block block{};
    for (std::size_t offset = 0; offset < payload.size(); offset += blockSize) {
        makeBlock(state, block);
        const std::size_t size = std::min(blockSize, payload.size() - offset);
        for (std::size_t i = 0; i < size; i++)
            payload[offset + i] ^= block[i];
        if (++state[counterLow] == 0)
            state[counterHigh]++;
    }
}

void quarterRound(State& x, std::size_t a, std::size_t b, std::size_t c, std::size_t d) {
    x[a] += x[b];
    x[d] = std::rotl(x[d] ^ x[a], 16);
    x[c] += x[d];
    x[b] = std::rotl(x[b] ^ x[c], 12);
    x[a] += x[b];
    x[d] = std::rotl(x[d] ^ x[a], 8);
    x[c] += x[d];
    x[b] = std::rotl(x[b] ^ x[c], 7);
}

void chaChaBlock(const State& state, std::uint32_t rounds, Block& block) {
    State working = state;
    for (std::uint32_t i = 0; i < rounds; i += 2) {
        quarterRound(working, 0, 4, 8, 12);
        quarterRound(working, 1, 5, 9, 13);
        quarterRound(working, 2, 6, 10, 14);
        quarterRound(working, 3, 7, 11, 15);
        quarterRound(working, 0, 5, 10, 15);
        quarterRound(working, 1, 6, 11, 12);
        quarterRound(working, 2, 7, 8, 13);
        quarterRound(working, 3, 4, 9, 14);
    }
    for (std::size_t i = 0; i < working.size(); i++)
        working[i] += state[i];
    writeWords(working, block);
}

void decryptChaCha(std::span<std::byte> payload, std::span<const std::byte> trailer, const AesKey& key, const CipherSpec& spec) {
    State state{};
    setConstants(state);
    setKeyHalves(state, key, spec.keyLayout);
    const std::size_t word = spec.nonceOffset / 4;
    switch (spec.nonceLayout) {
    case N::Nonce96:
        state[13] = readWord(trailer, word);
        state[14] = readWord(trailer, word + 1);
        state[15] = readWord(trailer, word + 2);
        break;
    case N::Nonce64WithFirstWord:
        state[13] = readWord(trailer, word);
        state[14] = readWord(trailer, word + 1);
        state[15] = readWord(trailer, 0);
        break;
    default:
        state[14] = readWord(trailer, word);
        state[15] = readWord(trailer, word + 1);
        break;
    }
    xorStream(payload, state, 12, 13, [&](const State& current, Block& block) { chaChaBlock(current, spec.rounds, block); });
}

void buildCustomPermutedState(State& state, std::span<const std::byte> trailer, const AesKey& key, const CipherSpec& spec) {
    const std::array<std::uint32_t, 8> k = readKey(key);
    const std::uint32_t nonce0 = readWord(trailer, spec.nonceOffset / 4);
    const std::uint32_t nonce1 = readWord(trailer, spec.nonceOffset / 4 + 1);
    setConstants(state);
    std::array<std::size_t, 8> order{};
    switch (spec.keyLayout) {
    case K::Swap16:
        order = {7, 3, 6, 2, 5, 1, 4, 0};
        break;
    case K::Second16Repeated:
        order = {7, 7, 6, 6, 5, 5, 4, 4};
        break;
    case K::First16Repeated:
        order = {3, 3, 2, 2, 1, 1, 0, 0};
        break;
    default:
        order = {3, 7, 2, 6, 1, 5, 0, 4};
        break;
    }
    state[4] = k[order[0]];
    state[5] = 0;
    state[6] = k[order[1]];
    state[7] = k[order[2]];
    state[8] = 0;
    state[9] = k[order[3]];
    state[10] = k[order[4]];
    state[11] = nonce1;
    state[12] = k[order[5]];
    state[13] = k[order[6]];
    state[14] = nonce0;
    state[15] = k[order[7]];
}

void customPermutedBlock(const State& state, std::uint32_t rounds, Block& block) {
    std::uint32_t v9 = state[8];
    std::uint32_t v10 = state[1];
    std::uint32_t v11 = state[2];
    std::uint32_t v64 = state[3];
    std::uint32_t v63 = state[4];
    std::uint32_t v62 = state[5];
    std::uint32_t v12 = state[6];
    std::uint32_t v58 = state[7];
    std::uint32_t v13 = state[9];
    std::uint32_t v59 = state[10];
    std::uint32_t v57 = state[11];
    std::uint32_t v14 = state[12];
    std::uint32_t v60 = state[14];
    std::uint32_t v15 = state[15];
    std::uint32_t v61 = state[13];
    std::uint32_t v17 = state[0];
    for (std::uint32_t round = 0; round < rounds; round++) {
        const std::uint32_t v18 = v63 ^ ror(v14 + v17, 25);
        const std::uint32_t v48 = v9 ^ ror(v18 + v17, 23);
        const std::uint32_t v47 = v14 ^ ror(v48 + v18, 19);
        const std::uint32_t v19 = v17 ^ ror(v48 + v47, 14);
        const std::uint32_t v20 = v62 ^ ror(v10 + v61, 25);
        const std::uint32_t v49 = v13 ^ ror(v20 + v10, 23);
        const std::uint32_t v21 = v61 ^ ror(v49 + v20, 19);
        const std::uint32_t v22 = v10 ^ ror(v49 + v21, 14);
        const std::uint32_t v50 = v12 ^ ror(v11 + v60, 25);
        const std::uint32_t v23 = v59 ^ ror(v50 + v11, 23);
        const std::uint32_t v24 = v60 ^ ror(v23 + v50, 19);
        const std::uint32_t v25 = v11 ^ ror(v24 + v23, 14);
        const std::uint32_t v26 = v58 ^ ror(v64 + v15, 25);
        const std::uint32_t v27 = v57 ^ ror(v26 + v64, 23);
        const std::uint32_t v28 = v15 ^ ror(v27 + v26, 19);
        const std::uint32_t v29 = v64 ^ ror(v28 + v27, 14);
        const std::uint32_t v30 = v21 ^ ror(v19 + v26, 25);
        const std::uint32_t v31 = v23 ^ ror(v30 + v19, 23);
        v61 = v30;
        v58 = v26 ^ ror(v31 + v30, 19);
        v59 = v31;
        v17 = v19 ^ ror(v58 + v31, 14);
        v60 = v24 ^ ror(v22 + v18, 25);
        v57 = v27 ^ ror(v60 + v22, 23);
        v63 = v18 ^ ror(v57 + v60, 19);
        v10 = v22 ^ ror(v57 + v63, 14);
        v15 = v28 ^ ror(v25 + v20, 25);
        v9 = v48 ^ ror(v15 + v25, 23);
        v62 = v20 ^ ror(v9 + v15, 19);
        v11 = v25 ^ ror(v9 + v62, 14);
        v14 = v47 ^ ror(v29 + v50, 25);
        v13 = v49 ^ ror(v14 + v29, 23);
        v12 = v50 ^ ror(v13 + v14, 19);
        v64 = v29 ^ ror(v12 + v13, 14);
    }
    const State output = {state[0] + v17, v61 + state[13], state[10] + v59, state[7] + v58, state[4] + v63, state[1] + v10,
                          state[14] + v60, state[11] + v57, state[8] + v9, state[5] + v62, state[2] + v11, state[15] + v15,
                          state[12] + v14, state[9] + v13, state[6] + v12, state[3] + v64};
    writeWords(output, block);
}

void customPermutedAltBlock(const State& state, std::uint32_t rounds, Block& block) {
    std::uint32_t v9 = state[8];
    std::uint32_t v87 = state[1];
    std::uint32_t v11 = state[2];
    std::uint32_t v12 = state[3];
    std::uint32_t v13 = state[4];
    std::uint32_t v14 = state[5];
    std::uint32_t v89 = state[6];
    std::uint32_t v84 = state[7];
    std::uint32_t v15 = state[9];
    std::uint32_t v85 = state[10];
    std::uint32_t v83 = state[11];
    std::uint32_t v16 = state[12];
    std::uint32_t v88 = state[13];
    std::uint32_t v18 = state[14];
    std::uint32_t v86 = state[15];
    std::uint32_t v20 = state[0];
    for (std::uint32_t round = 0; round < rounds; round++) {
        const std::uint32_t v21 = v13 ^ ror(v16 + v20, 25);
        const std::uint32_t v74 = v9 ^ ror(v21 + v20, 23);
        const std::uint32_t v73 = v16 ^ ror(v74 + v21, 19);
        const std::uint32_t v22 = v20 ^ ror(v74 + v73, 14);
        const std::uint32_t v76 = v14 ^ ror(v87 + v88, 25);
        const std::uint32_t v75 = v15 ^ ror(v76 + v87, 23);
        const std::uint32_t v23 = v88 ^ ror(v75 + v76, 19);
        const std::uint32_t v24 = v87 ^ ror(v75 + v23, 14);
        const std::uint32_t v25 = v89 ^ ror(v11 + v18, 25);
        const std::uint32_t v26 = v85 ^ ror(v25 + v11, 23);
        const std::uint32_t v28 = v18 ^ ror(v26 + v25, 19);
        const std::uint32_t v29 = v11 ^ ror(v28 + v26, 14);
        const std::uint32_t v30 = v84 ^ ror(v12 + v86, 25);
        const std::uint32_t v31 = v83 ^ ror(v30 + v12, 23);
        const std::uint32_t v32 = v86 ^ ror(v31 + v30, 19);
        const std::uint32_t v33 = v12 ^ ror(v32 + v31, 14);
        const std::uint32_t v34 = v23 ^ ror(v22 + v30, 25);
        const std::uint32_t v35 = v26 ^ ror(v34 + v22, 23);
        v88 = v34;
        v84 = v30 ^ ror(v35 + v34, 19);
        v85 = v35;
        v20 = v22 ^ ror(v84 + v35, 14);
        const std::uint32_t v36 = v28 ^ ror(v24 + v21, 25);
        const std::uint32_t v37 = v31 ^ ror(v36 + v24, 23);
        v18 = v36;
        v13 = v21 ^ ror(v37 + v36, 19);
        v83 = v37;
        v87 = v24 ^ ror(v13 + v37, 14);
        const std::uint32_t v38 = v32 ^ ror(v29 + v76, 25);
        v9 = v74 ^ ror(v38 + v29, 23);
        v86 = v38;
        v14 = v76 ^ ror(v9 + v38, 19);
        v11 = v29 ^ ror(v9 + v14, 14);
        v16 = v73 ^ ror(v33 + v25, 25);
        v15 = v75 ^ ror(v16 + v33, 23);
        v89 = v25 ^ ror(v15 + v16, 19);
        v12 = v33 ^ ror(v15 + v89, 14);
    }
    const State output = {state[0] + v20, v88 + state[13], state[10] + v85, state[7] + v84, state[4] + v13, state[1] + v87,
                          state[14] + v18, state[11] + v83, state[8] + v9, state[5] + v14, state[2] + v11, state[15] + v86,
                          state[12] + v16, state[9] + v15, state[6] + v89, state[3] + v12};
    writeWords(output, block);
}

std::array<std::uint32_t, 6> readSeed(std::span<const std::byte> trailer, NonceLayout layout) {
    std::size_t first = 0;
    if (layout == N::SalsaSeed234501)
        first = 2;
    else if (layout == N::SalsaSeed345012)
        first = 3;
    std::array<std::uint32_t, 6> seed{};
    for (std::size_t i = 0; i < seed.size(); i++)
        seed[i] = readWord(trailer, (first + i) % seed.size());
    return seed;
}

void customPermutedPremix(State& state, const std::array<std::uint32_t, 6>& seed, std::uint32_t rounds) {
    std::uint32_t v33 = seed[0];
    std::uint32_t v75 = seed[1];
    std::uint32_t v34 = seed[2];
    std::uint32_t v35 = seed[3];
    state[14] = seed[4];
    state[11] = seed[5];
    std::uint32_t v70 = state[13];
    std::uint32_t v69 = state[10];
    std::uint32_t v71 = state[7];
    std::uint32_t v36 = state[4];
    std::uint32_t v37 = state[15];
    std::uint32_t v38 = state[12];
    std::uint32_t v39 = state[9];
    std::uint32_t v76 = state[6];
    std::uint32_t v40 = state[0];
    std::uint32_t v41 = state[1];
    std::uint32_t v74 = state[2];
    std::uint32_t v42 = state[3];
    std::uint32_t v43 = v33;
    for (std::uint32_t round = 0; round < rounds; round++) {
        const std::uint32_t v68 = v36 ^ ror(v38 + v40, 25);
        const std::uint32_t v65 = v34 ^ ror(v68 + v40, 23);
        const std::uint32_t v64 = v38 ^ ror(v65 + v68, 19);
        const std::uint32_t v45 = v40 ^ ror(v65 + v64, 14);
        const std::uint32_t v67 = v35 ^ ror(v70 + v41, 25);
        const std::uint32_t v66 = v39 ^ ror(v67 + v41, 23);
        const std::uint32_t v46 = v70 ^ ror(v66 + v67, 19);
        const std::uint32_t v47 = v41 ^ ror(v46 + v66, 14);
        const std::uint32_t v77 = v76 ^ ror(v43 + v74, 25);
        const std::uint32_t v48 = v69 ^ ror(v77 + v74, 23);
        const std::uint32_t v49 = v43 ^ ror(v48 + v77, 19);
        const std::uint32_t v50 = v74 ^ ror(v49 + v48, 14);
        const std::uint32_t v51 = v71 ^ ror(v37 + v42, 25);
        const std::uint32_t v53 = v75 ^ ror(v51 + v42, 23);
        const std::uint32_t v54 = v37 ^ ror(v53 + v51, 19);
        const std::uint32_t v55 = v42 ^ ror(v54 + v53, 14);
        v70 = v46 ^ ror(v45 + v51, 25);
        v69 = v48 ^ ror(v70 + v45, 23);
        v71 = v51 ^ ror(v69 + v70, 19);
        v40 = v45 ^ ror(v71 + v69, 14);
        const std::uint32_t v56 = v49 ^ ror(v47 + v68, 25);
        const std::uint32_t v57 = v53 ^ ror(v56 + v47, 23);
        v43 = v56;
        v36 = v68 ^ ror(v57 + v56, 19);
        v75 = v57;
        const std::uint32_t v58 = v47 ^ ror(v36 + v57, 14);
        v37 = v54 ^ ror(v50 + v67, 25);
        v34 = v65 ^ ror(v37 + v50, 23);
        v35 = v67 ^ ror(v34 + v37, 19);
        v74 = v50 ^ ror(v35 + v34, 14);
        v38 = v64 ^ ror(v77 + v55, 25);
        v39 = v66 ^ ror(v38 + v55, 23);
        v76 = v77 ^ ror(v39 + v38, 19);
        v42 = v55 ^ ror(v39 + v76, 14);
        v41 = v58;
    }
    state[13] = v40;
    state[10] = v41;
    state[7] = v74;
    state[4] = v42;
    state[15] = v43;
    state[12] = v75;
    state[9] = v34;
    state[6] = v35;
    state[5] = 0;
    state[8] = 0;
}

void buildCustomPermutedPremixedState(State& state, std::span<const std::byte> trailer, const AesKey& key, const CipherSpec& spec) {
    const std::array<std::uint32_t, 8> k = readKey(key);
    const std::array<std::uint32_t, 6> seed = readSeed(trailer, spec.nonceLayout == N::SalsaSeed345012 ? N::SalsaSeed345012 : N::SalsaSeed012345);
    setConstants(state);
    state[4] = k[3];
    state[5] = 0;
    state[6] = k[7];
    state[7] = k[2];
    state[8] = 0;
    state[9] = k[6];
    state[10] = k[1];
    state[11] = seed[5];
    state[12] = k[5];
    state[13] = k[0];
    state[14] = seed[4];
    state[15] = k[4];
    customPermutedPremix(state, seed, spec.premixRounds);
}

State salsaRorCore(const State& state, std::uint32_t rounds) {
    std::uint32_t v160 = state[1];
    std::uint32_t v265 = state[2];
    std::uint32_t v266 = state[3];
    std::uint32_t v162 = state[4];
    std::uint32_t v267 = state[5];
    std::uint32_t v268 = state[6];
    std::uint32_t v163 = state[7];
    std::uint32_t v164 = state[8];
    std::uint32_t v245 = state[9];
    std::uint32_t v269 = state[10];
    std::uint32_t v263 = state[11];
    std::uint32_t v165 = state[12];
    std::uint32_t v166 = state[13];
    std::uint32_t v167 = state[14];
    std::uint32_t v243 = state[15];
    std::uint32_t v264 = state[0];
    for (std::uint32_t round = 0; round < rounds; round++) {
        const std::uint32_t v171 = ror((v162 + v264) ^ v165, 16);
        const std::uint32_t v173 = ror((v171 + v164) ^ v162, 20);
        const std::uint32_t v174 = v173 + v162 + v264;
        const std::uint32_t v289 = ror(v174 ^ v171, 24);
        const std::uint32_t v175 = v289 + v171 + v164;
        const std::uint32_t v290 = ror(v175 ^ v173, 25);
        const std::uint32_t v176 = ror((v267 + v160) ^ v166, 16);
        const std::uint32_t v177 = v176 + v245;
        const std::uint32_t v178 = ror(v177 ^ v267, 20);
        const std::uint32_t v179 = v178 + v267 + v160;
        v245 = ror(v179 ^ v176, 24);
        const std::uint32_t v180 = v245 + v177;
        const std::uint32_t v181 = ror(v180 ^ v178, 25);
        const std::uint32_t v182 = ror((v268 + v265) ^ v167, 16);
        const std::uint32_t v183 = ror((v182 + v269) ^ v268, 20);
        const std::uint32_t v184 = v183 + v268 + v265;
        const std::uint32_t v288 = ror(v184 ^ v182, 24);
        const std::uint32_t v185 = v288 + v182 + v269;
        const std::uint32_t v186 = ror(v185 ^ v183, 25);
        const std::uint32_t v188 = v163 + v266;
        const std::uint32_t v189 = ror(v188 ^ v243, 16);
        const std::uint32_t v190 = v189 + v263;
        const std::uint32_t v191 = ror(v190 ^ v163, 20);
        const std::uint32_t v192 = v191 + v188;
        const std::uint32_t v193 = ror(v192 ^ v189, 24);
        const std::uint32_t v194 = v193 + v190;
        const std::uint32_t v195 = ror(v194 ^ v191, 25);
        const std::uint32_t v196 = v181 + v174;
        const std::uint32_t v197 = ror(v196 ^ v193, 16);
        const std::uint32_t v198 = v197 + v185;
        const std::uint32_t v199 = ror(v198 ^ v181, 20);
        v264 = v199 + v196;
        v243 = ror(v264 ^ v197, 24);
        v269 = v243 + v198;
        v267 = ror(v269 ^ v199, 25);
        const std::uint32_t v201 = v186 + v179;
        const std::uint32_t v202 = ror(v201 ^ v289, 16);
        const std::uint32_t v203 = v202 + v194;
        const std::uint32_t v204 = ror(v203 ^ v186, 20);
        v160 = v204 + v201;
        const std::uint32_t v205 = ror(v160 ^ v202, 24);
        v263 = v205 + v203;
        v268 = ror(v263 ^ v204, 25);
        const std::uint32_t v206 = v195 + v184;
        const std::uint32_t v207 = ror(v206 ^ v245, 16);
        const std::uint32_t v208 = v207 + v175;
        const std::uint32_t v209 = ror(v208 ^ v195, 20);
        v165 = v205;
        v265 = v209 + v206;
        v166 = ror(v265 ^ v207, 24);
        v164 = v166 + v208;
        v163 = ror(v164 ^ v209, 25);
        const std::uint32_t v210 = v290 + v192;
        const std::uint32_t v211 = ror(v210 ^ v288, 16);
        const std::uint32_t v212 = v211 + v180;
        const std::uint32_t v213 = ror(v212 ^ v290, 20);
        v266 = v213 + v210;
        v167 = ror(v266 ^ v211, 24);
        v245 = v167 + v212;
        v162 = ror(v245 ^ v213, 25);
    }
    return {v264, v160, v265, v266, v162, v267, v268, v163, v164, v245, v269, v263, v165, v166, v167, v243};
}

void salsaRorBlock(const State& state, std::uint32_t rounds, Block& block) {
    State working = salsaRorCore(state, rounds);
    for (std::size_t i = 0; i < working.size(); i++)
        working[i] += state[i];
    writeWords(working, block);
}

void buildSalsaRorState(State& state, std::span<const std::byte> trailer, const AesKey& key, const CipherSpec& spec) {
    setConstants(state);
    setKeyHalves(state, key, spec.keyLayout);
    state[12] = 0;
    state[13] = 0;
    state[14] = readWord(trailer, spec.nonceOffset / 4);
    state[15] = readWord(trailer, spec.nonceOffset / 4 + 1);
}

void buildSalsaRorPremixedState(State& state, std::span<const std::byte> trailer, const AesKey& key, const CipherSpec& spec) {
    const std::array<std::uint32_t, 8> k = readKey(key);
    setConstants(state);
    for (std::size_t i = 0; i < 4; i++) {
        const bool second = spec.keyLayout == K::Second16Repeated;
        state[4 + i] = second ? k[4 + i] : k[i];
        state[8 + i] = k[4 + i];
    }
    const std::array<std::uint32_t, 6> seed = readSeed(trailer, spec.nonceLayout);
    for (std::size_t i = 0; i < 4; i++)
        state[12 + i] = seed[i];
    const State mixed = salsaRorCore(state, spec.premixRounds);
    setConstants(state);
    for (std::size_t i = 0; i < 4; i++) {
        state[4 + i] = mixed[i];
        state[8 + i] = mixed[12 + i];
    }
    state[12] = 0;
    state[13] = 0;
    state[14] = seed[4];
    state[15] = seed[5];
}

}

std::uint64_t getProSpiDescriptorLookupKey(std::span<const std::byte> trailer) {
    const std::array<std::uint32_t, 3> seedInput = {readWord(trailer, 0), readWord(trailer, 2), readWord(trailer, 4)};
    MersenneTwister64 generator;
    generator.seed(seedInput);
    const std::uint64_t secondSeed = generator.next() & 0xFFF;
    generator.seed(secondSeed);
    return generator.next();
}

bool decryptProSpi(std::span<std::byte> data, const AesKey& key) {
    if (data.size() < proSpiTrailerSize)
        return false;
    const std::span<std::byte> payload = data.first(data.size() - proSpiTrailerSize);
    const std::span<const std::byte> trailer = data.last(proSpiTrailerSize);
    const CipherSpec* spec = findSpec(trailer);
    if (spec == nullptr)
        return false;
    State state{};
    switch (spec->kind) {
    case C::ChaCha:
        decryptChaCha(payload, trailer, key, *spec);
        return true;
    case C::ProSpiCustomPermuted:
        buildCustomPermutedState(state, trailer, key, *spec);
        xorStream(payload, state, 8, 5, [&](const State& current, Block& block) { customPermutedBlock(current, spec->rounds, block); });
        return true;
    case C::ProSpiCustomPermutedAlt:
        buildCustomPermutedState(state, trailer, key, *spec);
        xorStream(payload, state, 8, 5, [&](const State& current, Block& block) { customPermutedAltBlock(current, spec->rounds, block); });
        return true;
    case C::ProSpiCustomPermutedPremixed:
        buildCustomPermutedPremixedState(state, trailer, key, *spec);
        xorStream(payload, state, 8, 5, [&](const State& current, Block& block) { customPermutedBlock(current, spec->rounds, block); });
        return true;
    case C::ProSpiSalsaRor:
        buildSalsaRorState(state, trailer, key, *spec);
        xorStream(payload, state, 12, 13, [&](const State& current, Block& block) { salsaRorBlock(current, spec->rounds, block); });
        return true;
    case C::ProSpiSalsaRorPremixed:
        buildSalsaRorPremixedState(state, trailer, key, *spec);
        xorStream(payload, state, 12, 13, [&](const State& current, Block& block) { salsaRorBlock(current, spec->rounds, block); });
        return true;
    }
    return false;
}

}
