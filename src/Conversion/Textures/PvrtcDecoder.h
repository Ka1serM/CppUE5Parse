#pragma once

#include <cstddef>

namespace ue5parse::conversion {

//! Decodes a PVRTC1 image of blocksX by blocksY twiddled 8-byte blocks into sizeX by sizeY RGBA pixels from its top left corner.
void decodePvrtc(const std::byte* source, int blocksX, int blocksY, bool twoBits, int sizeX, int sizeY, float* rgba);

}
