#pragma once

#include <cstddef>
#include <span>

namespace ue5parse {

//! Strinova XORs its Lua files with a fixed key.
void decryptStrinovaLua(std::span<std::byte> file);

}
