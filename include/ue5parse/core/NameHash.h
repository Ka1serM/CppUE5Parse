#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ue5parse {

// CityHash64 of the lowercase bytes of a text, as a constexpr function, so that FName constants are hashed at compile
// time. A port of CityHash 1.1 (external/cityhash/src/city.cc) that lowercases each byte as it reads it. hashName
// uses it at compile time and the library's CityHash64 at run time; tests check that both agree.
namespace namehash {

constexpr std::uint64_t k0 = 0xc3a5c85c97cb3127ull;
constexpr std::uint64_t k1 = 0xb492b66fbe98f273ull;
constexpr std::uint64_t k2 = 0x9ae16a3b2f90404full;

constexpr std::uint8_t lowerByte(std::string_view s, std::size_t i) {
    char c = s[i];
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return static_cast<std::uint8_t>(c);
}

constexpr std::uint64_t fetch64(std::string_view s, std::size_t at) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; i++) value |= std::uint64_t(lowerByte(s, at + i)) << (8 * i);
    return value;
}

constexpr std::uint64_t fetch32(std::string_view s, std::size_t at) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 4; i++) value |= std::uint64_t(lowerByte(s, at + i)) << (8 * i);
    return value;
}

constexpr std::uint64_t rotate(std::uint64_t value, int shift) {
    return shift == 0 ? value : ((value >> shift) | (value << (64 - shift)));
}

constexpr std::uint64_t byteSwap(std::uint64_t value) {
    std::uint64_t out = 0;
    for (int i = 0; i < 8; i++) out |= ((value >> (8 * i)) & 0xff) << (8 * (7 - i));
    return out;
}

constexpr std::uint64_t shiftMix(std::uint64_t value) { return value ^ (value >> 47); }

constexpr std::uint64_t hashLen16(std::uint64_t u, std::uint64_t v, std::uint64_t mul) {
    std::uint64_t a = (u ^ v) * mul;
    a ^= a >> 47;
    std::uint64_t b = (v ^ a) * mul;
    b ^= b >> 47;
    return b * mul;
}

constexpr std::uint64_t hashLen16(std::uint64_t u, std::uint64_t v) { return hashLen16(u, v, 0x9ddfea08eb382d69ull); }

constexpr std::uint64_t hashLen0to16(std::string_view s) {
    std::size_t len = s.size();
    if (len >= 8) {
        std::uint64_t mul = k2 + len * 2;
        std::uint64_t a = fetch64(s, 0) + k2;
        std::uint64_t b = fetch64(s, len - 8);
        std::uint64_t c = rotate(b, 37) * mul + a;
        std::uint64_t d = (rotate(a, 25) + b) * mul;
        return hashLen16(c, d, mul);
    }
    if (len >= 4) {
        std::uint64_t mul = k2 + len * 2;
        std::uint64_t a = fetch32(s, 0);
        return hashLen16(len + (a << 3), fetch32(s, len - 4), mul);
    }
    if (len > 0) {
        std::uint32_t a = lowerByte(s, 0);
        std::uint32_t b = lowerByte(s, len >> 1);
        std::uint32_t c = lowerByte(s, len - 1);
        std::uint32_t y = a + (b << 8);
        std::uint32_t z = static_cast<std::uint32_t>(len) + (c << 2);
        return shiftMix(y * k2 ^ z * k0) * k2;
    }
    return k2;
}

constexpr std::uint64_t hashLen17to32(std::string_view s) {
    std::size_t len = s.size();
    std::uint64_t mul = k2 + len * 2;
    std::uint64_t a = fetch64(s, 0) * k1;
    std::uint64_t b = fetch64(s, 8);
    std::uint64_t c = fetch64(s, len - 8) * mul;
    std::uint64_t d = fetch64(s, len - 16) * k2;
    return hashLen16(rotate(a + b, 43) + rotate(c, 30) + d, a + rotate(b + k2, 18) + c, mul);
}

constexpr std::uint64_t hashLen33to64(std::string_view s) {
    std::size_t len = s.size();
    std::uint64_t mul = k2 + len * 2;
    std::uint64_t a = fetch64(s, 0) * k2;
    std::uint64_t b = fetch64(s, 8);
    std::uint64_t c = fetch64(s, len - 24);
    std::uint64_t d = fetch64(s, len - 32);
    std::uint64_t e = fetch64(s, 16) * k2;
    std::uint64_t f = fetch64(s, 24) * 9;
    std::uint64_t g = fetch64(s, len - 8);
    std::uint64_t h = fetch64(s, len - 16) * mul;
    std::uint64_t u = rotate(a + g, 43) + (rotate(b, 30) + c) * 9;
    std::uint64_t v = ((a + g) ^ d) + f + 1;
    std::uint64_t w = byteSwap((u + v) * mul) + h;
    std::uint64_t x = rotate(e + f, 42) + c;
    std::uint64_t y = (byteSwap((v + w) * mul) + g) * mul;
    std::uint64_t z = e + f + c;
    a = byteSwap((x + z) * mul + y) + b;
    b = shiftMix((z + a) * mul + d + h) * mul;
    return b + x;
}

struct Pair {
    std::uint64_t first;
    std::uint64_t second;
};

constexpr Pair weakHashLen32WithSeeds(std::string_view s, std::size_t at, std::uint64_t a, std::uint64_t b) {
    std::uint64_t w = fetch64(s, at);
    std::uint64_t x = fetch64(s, at + 8);
    std::uint64_t y = fetch64(s, at + 16);
    std::uint64_t z = fetch64(s, at + 24);
    a += w;
    b = rotate(b + a + z, 21);
    std::uint64_t c = a;
    a += x;
    a += y;
    b += rotate(a, 44);
    return Pair{a + z, b + c};
}

constexpr std::uint64_t cityHash64Lower(std::string_view s) {
    std::size_t len = s.size();
    if (len <= 16) return hashLen0to16(s);
    if (len <= 32) return hashLen17to32(s);
    if (len <= 64) return hashLen33to64(s);

    std::uint64_t x = fetch64(s, len - 40);
    std::uint64_t y = fetch64(s, len - 16) + fetch64(s, len - 56);
    std::uint64_t z = hashLen16(fetch64(s, len - 48) + len, fetch64(s, len - 24));
    Pair v = weakHashLen32WithSeeds(s, len - 64, len, z);
    Pair w = weakHashLen32WithSeeds(s, len - 32, y + k1, x);
    x = x * k1 + fetch64(s, 0);

    std::size_t at = 0;
    std::size_t remaining = (len - 1) & ~std::size_t(63);
    do {
        x = rotate(x + y + v.first + fetch64(s, at + 8), 37) * k1;
        y = rotate(y + v.second + fetch64(s, at + 48), 42) * k1;
        x ^= w.second;
        y += v.first + fetch64(s, at + 40);
        z = rotate(z + w.first, 33) * k1;
        v = weakHashLen32WithSeeds(s, at, v.second * k1, x + w.first);
        w = weakHashLen32WithSeeds(s, at + 32, z + w.second, y + fetch64(s, at + 16));
        std::uint64_t swap = z;
        z = x;
        x = swap;
        at += 64;
        remaining -= 64;
    } while (remaining != 0);
    return hashLen16(hashLen16(v.first, w.first) + shiftMix(y) * k1 + z, hashLen16(v.second, w.second) + x);
}

}

}
