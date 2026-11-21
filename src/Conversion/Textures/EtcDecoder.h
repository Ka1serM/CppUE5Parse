#pragma once

#include <cstddef>

namespace ue5parse::conversion {

void decodeEtc2Rgb(const std::byte* source, float* rgba);
void decodeEtc2Rgba(const std::byte* source, float* rgba);
void decodeEacR11(const std::byte* source, float* rgba);
void decodeEacRg11(const std::byte* source, float* rgba);

}
