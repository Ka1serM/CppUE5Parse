#pragma once

#include <cstddef>
#include <span>

namespace ue5parse {

//! Snowbreak encrypts its Lua files with AES-256 ECB up to the last full block, without padding.
void decryptSnowbreakLua(std::span<std::byte> file);

}
