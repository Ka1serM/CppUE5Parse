#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "Conversion/Error.h"

namespace ue5parse::conversion {

struct Sound {
    int channels = 0;
    int sampleRate = 0;
    std::vector<std::int16_t> samples;
};

ConversionResult<Sound> decodeSound(std::span<const std::byte> file, std::string_view fileName);

}
