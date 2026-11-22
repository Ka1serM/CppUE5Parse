#pragma once

#include <cstddef>
#include <span>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

//! Dragon Sword XORs each index string with its last character and the encoded entries with their sixth byte; these decode the index in place.
Result<void> decodeDragonSwordPrimaryIndex(std::span<std::byte> index);
Result<void> decodeDragonSwordDirectoryIndex(std::span<std::byte> index);

}
