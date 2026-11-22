#include "CppUE5Parse/GameTypes/TonyHawkProSkater12/TonyHawkProSkater12Aes.h"

#include <array>
#include <cstdint>
#include <cstring>

#include "CppUE5Parse/Crypto/Rijndael.h"

namespace ue5parse {

namespace {

using Rijndael::td0;
using Rijndael::td1;
using Rijndael::td2;
using Rijndael::td3;
using Rijndael::td4;

//! The game decrypts with these fixed round keys and does not use an AES key.
constexpr std::array<std::uint32_t, 60> rk = {
    0x47269fd9u, 0x25e25346u, 0x338ad3e4u, 0x6c53a789u,
    0x5407ba92u, 0xc46782fdu, 0xabb54f69u, 0x456919bau,
    0x3e13aaabu, 0xa7bff815u, 0x0e2c205eu, 0xef75e1e4u,
    0xc96d860du, 0x9060386fu, 0x6fd2cd94u, 0xeedc56d3u,
    0xed789294u, 0x99ac52beu, 0xa993d84bu, 0xe159c1bau,
    0x96b60cc5u, 0x590dbe62u, 0xffb2f5fbu, 0x810e9b47u,
    0xd23a67c7u, 0x74d4c02au, 0x303f8af5u, 0x48ca19f1u,
    0x011d7ae9u, 0xcfbbb2a7u, 0xa6bf4b99u, 0x7ebc6ebcu,
    0xdf516cc2u, 0xa6eea7edu, 0x44eb4adfu, 0x78f59304u,
    0x492df62bu, 0xcea6c84eu, 0x6904f93eu, 0xd8032525u,
    0x669cff94u, 0x79bfcb2fu, 0xe205ed32u, 0x3c1ed9dbu,
    0xcdc5fb27u, 0x878b3e65u, 0xa7a23170u, 0xb107dc1bu,
    0x7ccf0607u, 0x1f2334bbu, 0x9bba261du, 0xde1b34e9u,
    0x1ed7860au, 0x4a4ec542u, 0x20290f15u, 0x16a5ed6bu,
    0xeabd3620u, 0x674a3418u, 0x173dc84bu, 0x94dec78fu,
};

std::uint32_t loadBigEndian(std::span<const std::uint8_t, 16> bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(bytes[offset]) << 24) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8)
           | bytes[offset + 3];
}

void storeBigEndian(std::span<std::uint8_t, 16> bytes, std::size_t offset, std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 24);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 16);
    bytes[offset + 2] = static_cast<std::uint8_t>(value >> 8);
    bytes[offset + 3] = static_cast<std::uint8_t>(value);
}

//! The game keeps the second byte of one state word from an earlier round in the last rounds.
std::uint32_t replace(std::uint32_t input, std::uint32_t output) {
    return (output & 0xFF00FFFF) | (input & 0x00FF0000);
}

void decryptBlock(std::span<const std::uint8_t, 16> ciphertext, std::span<std::uint8_t, 16> plaintext) {
    std::uint32_t s0 = loadBigEndian(ciphertext, 0) ^ rk[0];
    std::uint32_t s1 = loadBigEndian(ciphertext, 4) ^ rk[1];
    std::uint32_t s2 = loadBigEndian(ciphertext, 8) ^ rk[2];
    std::uint32_t s3 = loadBigEndian(ciphertext, 12) ^ rk[3];
    std::uint32_t t0 = td0[s0 >> 24] ^ td1[(s3 >> 16) & 0xff] ^ td2[(s2 >> 8) & 0xff] ^ td3[s1 & 0xff] ^ rk[4];
    std::uint32_t t1 = td0[s1 >> 24] ^ td1[(s0 >> 16) & 0xff] ^ td2[(s3 >> 8) & 0xff] ^ td3[s2 & 0xff] ^ rk[5];
    std::uint32_t t2 = td0[s2 >> 24] ^ td1[(s1 >> 16) & 0xff] ^ td2[(s0 >> 8) & 0xff] ^ td3[s3 & 0xff] ^ rk[6];
    std::uint32_t t3 = td0[s3 >> 24] ^ td1[(s2 >> 16) & 0xff] ^ td2[(s1 >> 8) & 0xff] ^ td3[s0 & 0xff] ^ rk[7];
    s0 = td0[t0 >> 24] ^ td1[(t3 >> 16) & 0xff] ^ td2[(t2 >> 8) & 0xff] ^ td3[t1 & 0xff] ^ rk[8];
    s1 = td0[t1 >> 24] ^ td1[(t0 >> 16) & 0xff] ^ td2[(t3 >> 8) & 0xff] ^ td3[t2 & 0xff] ^ rk[9];
    s2 = td0[t2 >> 24] ^ td1[(t1 >> 16) & 0xff] ^ td2[(t0 >> 8) & 0xff] ^ td3[t3 & 0xff] ^ rk[10];
    s3 = td0[t3 >> 24] ^ td1[(t2 >> 16) & 0xff] ^ td2[(t1 >> 8) & 0xff] ^ td3[t0 & 0xff] ^ rk[11];
    t0 = td0[s0 >> 24] ^ td1[(s3 >> 16) & 0xff] ^ td2[(s2 >> 8) & 0xff] ^ td3[s1 & 0xff] ^ rk[12];
    t1 = td0[s1 >> 24] ^ td1[(s0 >> 16) & 0xff] ^ td2[(s3 >> 8) & 0xff] ^ td3[s2 & 0xff] ^ rk[13];
    t2 = td0[s2 >> 24] ^ td1[(s1 >> 16) & 0xff] ^ td2[(s0 >> 8) & 0xff] ^ td3[s3 & 0xff] ^ rk[14];
    t3 = td0[s3 >> 24] ^ td1[(s2 >> 16) & 0xff] ^ td2[(s1 >> 8) & 0xff] ^ td3[s0 & 0xff] ^ rk[15];
    s0 = td0[t0 >> 24] ^ td1[(t3 >> 16) & 0xff] ^ td2[(t2 >> 8) & 0xff] ^ td3[t1 & 0xff] ^ rk[16];
    s1 = td0[t1 >> 24] ^ td1[(t0 >> 16) & 0xff] ^ td2[(t3 >> 8) & 0xff] ^ td3[t2 & 0xff] ^ rk[17];
    s2 = td0[t2 >> 24] ^ td1[(t1 >> 16) & 0xff] ^ td2[(t0 >> 8) & 0xff] ^ td3[t3 & 0xff] ^ rk[18];
    s3 = td0[t3 >> 24] ^ td1[(t2 >> 16) & 0xff] ^ td2[(t1 >> 8) & 0xff] ^ td3[t0 & 0xff] ^ rk[19];
    std::uint32_t tk = replace(s2, t2);
    t0 = td0[s0 >> 24] ^ td1[(s3 >> 16) & 0xff] ^ td2[(s2 >> 8) & 0xff] ^ td3[s1 & 0xff] ^ rk[20];
    t1 = td0[s1 >> 24] ^ td1[(s0 >> 16) & 0xff] ^ td2[(s3 >> 8) & 0xff] ^ td3[s2 & 0xff] ^ rk[21];
    t2 = td0[s2 >> 24] ^ td1[(s1 >> 16) & 0xff] ^ td2[(s0 >> 8) & 0xff] ^ td3[s3 & 0xff] ^ rk[22];
    t3 = td0[s3 >> 24] ^ td1[(tk >> 16) & 0xff] ^ td2[(s1 >> 8) & 0xff] ^ td3[s0 & 0xff] ^ rk[23];
    tk = replace(t2, tk);
    s0 = td0[t0 >> 24] ^ td1[(t3 >> 16) & 0xff] ^ td2[(t2 >> 8) & 0xff] ^ td3[t1 & 0xff] ^ rk[24];
    s1 = td0[t1 >> 24] ^ td1[(t0 >> 16) & 0xff] ^ td2[(t3 >> 8) & 0xff] ^ td3[t2 & 0xff] ^ rk[25];
    s2 = td0[t2 >> 24] ^ td1[(t1 >> 16) & 0xff] ^ td2[(t0 >> 8) & 0xff] ^ td3[t3 & 0xff] ^ rk[26];
    s3 = td0[t3 >> 24] ^ td1[(tk >> 16) & 0xff] ^ td2[(t1 >> 8) & 0xff] ^ td3[t0 & 0xff] ^ rk[27];
    tk = replace(s2, tk);
    t0 = td0[s0 >> 24] ^ td1[(s3 >> 16) & 0xff] ^ td2[(s2 >> 8) & 0xff] ^ td3[s1 & 0xff] ^ rk[28];
    t1 = td0[s1 >> 24] ^ td1[(s0 >> 16) & 0xff] ^ td2[(s3 >> 8) & 0xff] ^ td3[s2 & 0xff] ^ rk[29];
    t2 = td0[s2 >> 24] ^ td1[(s1 >> 16) & 0xff] ^ td2[(s0 >> 8) & 0xff] ^ td3[s3 & 0xff] ^ rk[30];
    t3 = td0[s3 >> 24] ^ td1[(tk >> 16) & 0xff] ^ td2[(s1 >> 8) & 0xff] ^ td3[s0 & 0xff] ^ rk[31];
    s0 = td0[t0 >> 24] ^ td1[(t3 >> 16) & 0xff] ^ td2[(t2 >> 8) & 0xff] ^ td3[t1 & 0xff] ^ rk[32];
    s1 = td0[t1 >> 24] ^ td1[(t0 >> 16) & 0xff] ^ td2[(t3 >> 8) & 0xff] ^ td3[t2 & 0xff] ^ rk[33];
    s2 = td0[t2 >> 24] ^ td1[(t1 >> 16) & 0xff] ^ td2[(t0 >> 8) & 0xff] ^ td3[t3 & 0xff] ^ rk[34];
    s3 = td0[t3 >> 24] ^ td1[(t2 >> 16) & 0xff] ^ td2[(t1 >> 8) & 0xff] ^ td3[t0 & 0xff] ^ rk[35];
    t0 = td0[s0 >> 24] ^ td1[(s3 >> 16) & 0xff] ^ td2[(s2 >> 8) & 0xff] ^ td3[s1 & 0xff] ^ rk[36];
    t1 = td0[s1 >> 24] ^ td1[(s0 >> 16) & 0xff] ^ td2[(s3 >> 8) & 0xff] ^ td3[s2 & 0xff] ^ rk[37];
    t2 = td0[s2 >> 24] ^ td1[(s1 >> 16) & 0xff] ^ td2[(s0 >> 8) & 0xff] ^ td3[s3 & 0xff] ^ rk[38];
    t3 = td0[s3 >> 24] ^ td1[(s2 >> 16) & 0xff] ^ td2[(s1 >> 8) & 0xff] ^ td3[s0 & 0xff] ^ rk[39];
    {
        s0 = td0[t0 >> 24] ^ td1[(t3 >> 16) & 0xff] ^ td2[(t2 >> 8) & 0xff] ^ td3[t1 & 0xff] ^ rk[40];
        s1 = td0[t1 >> 24] ^ td1[(t0 >> 16) & 0xff] ^ td2[(t3 >> 8) & 0xff] ^ td3[t2 & 0xff] ^ rk[41];
        s2 = td0[t2 >> 24] ^ td1[(t1 >> 16) & 0xff] ^ td2[(t0 >> 8) & 0xff] ^ td3[t3 & 0xff] ^ rk[42];
        s3 = td0[t3 >> 24] ^ td1[(t2 >> 16) & 0xff] ^ td2[(t1 >> 8) & 0xff] ^ td3[t0 & 0xff] ^ rk[43];
        t0 = td0[s0 >> 24] ^ td1[(s3 >> 16) & 0xff] ^ td2[(s2 >> 8) & 0xff] ^ td3[s1 & 0xff] ^ rk[44];
        t1 = td0[s1 >> 24] ^ td1[(s0 >> 16) & 0xff] ^ td2[(s3 >> 8) & 0xff] ^ td3[s2 & 0xff] ^ rk[45];
        t2 = td0[s2 >> 24] ^ td1[(s1 >> 16) & 0xff] ^ td2[(s0 >> 8) & 0xff] ^ td3[s3 & 0xff] ^ rk[46];
        t3 = td0[s3 >> 24] ^ td1[(s2 >> 16) & 0xff] ^ td2[(s1 >> 8) & 0xff] ^ td3[s0 & 0xff] ^ rk[47];
        {
            s0 = td0[t0 >> 24] ^ td1[(t3 >> 16) & 0xff] ^ td2[(t2 >> 8) & 0xff] ^ td3[t1 & 0xff] ^ rk[48];
            s1 = td0[t1 >> 24] ^ td1[(t0 >> 16) & 0xff] ^ td2[(t3 >> 8) & 0xff] ^ td3[t2 & 0xff] ^ rk[49];
            s2 = td0[t2 >> 24] ^ td1[(t1 >> 16) & 0xff] ^ td2[(t0 >> 8) & 0xff] ^ td3[t3 & 0xff] ^ rk[50];
            s3 = td0[t3 >> 24] ^ td1[(t2 >> 16) & 0xff] ^ td2[(t1 >> 8) & 0xff] ^ td3[t0 & 0xff] ^ rk[51];
            t0 = td0[s0 >> 24] ^ td1[(s3 >> 16) & 0xff] ^ td2[(s2 >> 8) & 0xff] ^ td3[s1 & 0xff] ^ rk[52];
            t1 = td0[s1 >> 24] ^ td1[(s0 >> 16) & 0xff] ^ td2[(s3 >> 8) & 0xff] ^ td3[s2 & 0xff] ^ rk[53];
            t2 = td0[s2 >> 24] ^ td1[(s1 >> 16) & 0xff] ^ td2[(s0 >> 8) & 0xff] ^ td3[s3 & 0xff] ^ rk[54];
            t3 = td0[s3 >> 24] ^ td1[(s2 >> 16) & 0xff] ^ td2[(s1 >> 8) & 0xff] ^ td3[s0 & 0xff] ^ rk[55];
        }
    }
    const std::uint32_t* last = rk.data() + 56;
    s0 =
        (td4[(t0 >> 24) & 0xff] & 0xff000000) ^
        (td4[(t3 >> 16) & 0xff] & 0x00ff0000) ^
        (td4[(t2 >> 8) & 0xff] & 0x0000ff00) ^
        (td4[(t1 >> 0) & 0xff] & 0x000000ff) ^
        last[0];
    storeBigEndian(plaintext, 0, s0);
    s1 =
        (td4[(t1 >> 24) & 0xff] & 0xff000000) ^
        (td4[(t0 >> 16) & 0xff] & 0x00ff0000) ^
        (td4[(t3 >> 8) & 0xff] & 0x0000ff00) ^
        (td4[(t2 >> 0) & 0xff] & 0x000000ff) ^
        last[1];
    storeBigEndian(plaintext, 4, s1);
    plaintext[7] = (std::uint8_t)(last[1] ^ td4[(std::uint8_t)t2]);
    s2 =
        (td4[(t2 >> 24) & 0xff] & 0xff000000) ^
        (td4[(t1 >> 16) & 0xff] & 0x00ff0000) ^
        (td4[(t0 >> 8) & 0xff] & 0x0000ff00) ^
        (td4[(t3 >> 0) & 0xff] & 0x000000ff) ^
        last[2];
    storeBigEndian(plaintext, 8, s2);
    plaintext[11] = (std::uint8_t)(last[2] ^ td4[(std::uint8_t)t3]);
    s3 =
        (td4[(t3 >> 24) & 0xff] & 0xff000000) ^
        (td4[(t2 >> 16) & 0xff] & 0x00ff0000) ^
        (td4[(t1 >> 8) & 0xff] & 0x0000ff00) ^
        (td4[(t0 >> 0) & 0xff] & 0x000000ff) ^
        last[3];
    storeBigEndian(plaintext, 12, s3);
}

}

void decryptTonyHawkProSkater12(std::span<std::byte> data) {
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        std::array<std::uint8_t, 16> input{};
        std::memcpy(input.data(), data.data() + offset, 16);
        decryptBlock(input, std::span<std::uint8_t, 16>(reinterpret_cast<std::uint8_t*>(data.data() + offset), 16));
    }
}

}
