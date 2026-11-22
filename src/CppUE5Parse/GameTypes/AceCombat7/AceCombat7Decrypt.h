#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ue5parse {

//! ACE7Key.bin of CUE4Parse and UAssetAPI: 217 rows of 1024 bytes.
inline constexpr std::size_t aceCombat7KeySize = 217 * 1024;

struct AceCombat7Stream {
    std::uint32_t row = 0;
    std::uint32_t column = 0;
};

//! The key position of a package from the file name without folder and extension.
AceCombat7Stream getAceCombat7Stream(std::string_view fileName);

//! The uasset is encrypted from byte 4 at stream position 0 and the uexp continues the stream; the caller restores the package tags.
void decryptAceCombat7(std::span<std::byte> data, AceCombat7Stream start, std::uint64_t position, std::span<const std::byte, aceCombat7KeySize> key);

}
