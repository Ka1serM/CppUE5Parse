#pragma once

#include <cstddef>
#include <span>

#include "CppUE5Parse/Crypto/Decryptor.h"

namespace ue5parse {

void decryptMindsEye(std::span<std::byte> data, const AesKey& key, bool isIndex);

}
