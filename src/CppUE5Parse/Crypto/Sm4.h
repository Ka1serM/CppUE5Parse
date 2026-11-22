#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ue5parse::Sm4 {

using Sbox = std::array<std::uint8_t, 256>;
using RoundKeys = std::array<std::uint32_t, 32>;

RoundKeys expandKey(std::span<const std::byte, 16> key, const Sbox& sbox, const std::array<std::uint32_t, 4>& familyKey,
                    std::span<const std::uint32_t, 32> constantKey);
void decrypt(std::span<std::byte> data, const RoundKeys& roundKeys, const Sbox& sbox);

}
