#pragma once

#include <cstdint>
#include <span>

namespace ue5parse {

enum class CodeVein2StringKey : std::uint8_t { Locres, StringTable };

//! Code Vein II XORs every character but the terminator with a key byte that the string length and the index select.
void decryptCodeVein2String(std::span<std::uint8_t> characters, CodeVein2StringKey key);
//! The wide strings also store 0xF000 for 0 and 0xF001 for 0xFFFF before the XOR.
void decryptCodeVein2String(std::span<char16_t> characters, CodeVein2StringKey key);

}
