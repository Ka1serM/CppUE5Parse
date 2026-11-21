#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "Conversion/Error.h"
#include "Engine/Source/Runtime/Core/PixelFormat.h"

namespace ue5parse::conversion {

struct PixelLayout {
    runtime::EPixelFormat format = runtime::EPixelFormat::PF_Unknown;
    int sizeX = 0;
    int sizeY = 0;
    int slices = 1;

    std::size_t getSliceSize() const;
    std::size_t getSize() const { return getSliceSize() * static_cast<std::size_t>(slices); }
};

struct DecodedImage {
    int sizeX = 0;
    int sizeY = 0;
    int slices = 0;
    std::vector<float> rgba;

    std::span<const float, 4> getPixel(int x, int y, int slice) const {
        return std::span<const float, 4>(rgba.data() + ((static_cast<std::size_t>(slice) * sizeY + y) * sizeX + x) * 4, 4);
    }
};

bool isDecodable(runtime::EPixelFormat format);
ConversionResult<DecodedImage> decodePixels(const PixelLayout& layout, std::span<const std::byte> data);

}
