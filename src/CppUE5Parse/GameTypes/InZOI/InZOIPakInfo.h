#pragma once

#include <cstddef>
#include <span>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

inline constexpr std::size_t inZOIEncryptedFooterSize = 263;

//! inZOI signs the last 256 bytes of the pak footer with RSA; the public key is the public_key.pem file next to the paks.
Result<void> decryptInZOIPakInfo(std::span<std::byte> tail, std::span<const std::byte> publicKeyPem);

}
