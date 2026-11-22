#pragma once

#include <cstddef>
#include <span>

#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

void decryptSplitgate2(std::span<std::byte> data, const AesKey& key);

}
