#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

struct ProSpiDescriptorEntry {
    std::uint64_t lookupKey;
    std::uint64_t descriptor;
};

inline constexpr std::size_t proSpiDescriptorCount = 4096;
//! Sorted by the lookup key.
extern const std::array<ProSpiDescriptorEntry, proSpiDescriptorCount> proSpiDescriptorTable;

//! eBaseball Pro Spirit puts a 0x18 byte trailer after each encrypted buffer; the trailer selects the cipher.
inline constexpr std::size_t proSpiTrailerSize = 0x18;

//! The lookup key that the trailer selects with two MT19937-64 generators.
std::uint64_t getProSpiDescriptorLookupKey(std::span<const std::byte> trailer);

//! Decrypts the data before the trailer in place; false when the cipher of the trailer is not known.
bool decryptProSpi(std::span<std::byte> data, const AesKey& key);

}
