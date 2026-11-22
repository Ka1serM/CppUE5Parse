#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

//! Neverness to Everness stores ini lines as base64 of AES-256 ECB data with PKCS7 padding; a line can hold several lines joined by |SPLIT|.
Result<std::vector<std::byte>> decryptNevernessToEvernessIni(std::span<const std::byte> file);

}
