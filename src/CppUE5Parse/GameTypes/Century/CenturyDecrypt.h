#pragma once

#include <cstddef>
#include <span>

namespace ue5parse {

//! Century: Age of Ashes stores PWC blocks as LZ4 data that is XORed with a chain of its own bytes after AES decryption.
void decryptCenturyPwc(std::span<std::byte> block);

}
