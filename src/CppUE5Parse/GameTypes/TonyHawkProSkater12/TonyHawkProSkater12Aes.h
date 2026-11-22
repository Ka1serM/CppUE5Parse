#pragma once

#include <cstddef>
#include <span>

#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

void decryptTonyHawkProSkater12(std::span<std::byte> data);

}
