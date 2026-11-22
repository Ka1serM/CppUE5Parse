#pragma once

#include <string>
#include <string_view>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

//! Neverness to Everness saves a localized string as URL-safe base64 of AES-256 ECB data, and the text ends at the first "HottaLocresSplit".
Result<std::string> decryptNevernessToEvernessLocresString(std::string_view base64);

}
