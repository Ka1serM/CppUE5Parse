#pragma once

#include <cstddef>
#include <span>

namespace ue5parse {

//! Game for Peace XORs compressed ini files that start with a known value; other files stay as they are.
void decryptGameForPeaceIni(std::span<std::byte> file);

}
