#include "Conversion/Textures/TextureDecoder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>

#define BCDEC_IMPLEMENTATION
#define BCDEC_STATIC
#include <bcdec.h>

namespace ue5parse::conversion {

namespace {

using runtime::EPixelFormat;

constexpr int blockPixels = 16;
using Block = std::array<float, blockPixels * 4>;
using DecodeBlock = void (*)(const std::byte* source, float* rgba);

struct FormatInfo {
    int blockSize = 1;
    int blockBytes = 0;
    DecodeBlock decode = nullptr;
};

template <class T>
T load(const std::byte* source) {
    T value;
    std::memcpy(&value, source, sizeof(T));
    return value;
}

float unorm8(std::uint8_t value) { return static_cast<float>(value) / 255.0f; }

float unorm16(std::uint16_t value) { return static_cast<float>(value) / 65535.0f; }

float unormBits(std::uint32_t value, int bits) { return static_cast<float>(value) / static_cast<float>((1u << bits) - 1u); }

float smallFloat(std::uint32_t bits, int mantissaBits) {
    const std::uint32_t mantissa = bits & ((1u << mantissaBits) - 1u);
    const int exponent = static_cast<int>(bits >> mantissaBits) & 0x1f;
    if (exponent == 0)
        return std::ldexp(static_cast<float>(mantissa), -14 - mantissaBits);
    if (exponent == 31)
        return mantissa == 0 ? INFINITY : NAN;
    return std::ldexp(1.0f + static_cast<float>(mantissa) / static_cast<float>(1u << mantissaBits), exponent - 15);
}

float half(std::uint16_t bits) {
    const float magnitude = smallFloat(bits & 0x7fffu, 10);
    return (bits & 0x8000u) != 0 ? -magnitude : magnitude;
}

void setPixel(float* rgba, float r, float g, float b, float a) {
    rgba[0] = r;
    rgba[1] = g;
    rgba[2] = b;
    rgba[3] = a;
}

void setGray(float* rgba, float value) { setPixel(rgba, value, value, value, 1.0f); }

void decodeBytes(std::span<const std::uint8_t, blockPixels * 4> bytes, float* rgba) {
    for (std::size_t i = 0; i < bytes.size(); i++)
        rgba[i] = unorm8(bytes[i]);
}

void decodeBc1(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 4> bytes;
    bcdec_bc1(source, bytes.data(), 16);
    decodeBytes(bytes, rgba);
}

void decodeBc2(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 4> bytes;
    bcdec_bc2(source, bytes.data(), 16);
    decodeBytes(bytes, rgba);
}

void decodeBc3(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 4> bytes;
    bcdec_bc3(source, bytes.data(), 16);
    decodeBytes(bytes, rgba);
}

void decodeBc7(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 4> bytes;
    bcdec_bc7(source, bytes.data(), 16);
    decodeBytes(bytes, rgba);
}

void decodeBc4(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels> red;
    bcdec_bc4(source, red.data(), 4);
    for (int i = 0; i < blockPixels; i++)
        setGray(rgba + i * 4, unorm8(red[i]));
}

void decodeBc5(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 2> redGreen;
    bcdec_bc5(source, redGreen.data(), 8);
    for (int i = 0; i < blockPixels; i++)
        setPixel(rgba + i * 4, unorm8(redGreen[i * 2]), unorm8(redGreen[i * 2 + 1]), 0.0f, 1.0f);
}

void decodeBc6h(const std::byte* source, float* rgba) {
    std::array<float, blockPixels * 3> rgb;
    bcdec_bc6h_float(source, rgb.data(), 4 * 3, 0);
    for (int i = 0; i < blockPixels; i++)
        setPixel(rgba + i * 4, rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2], 1.0f);
}

void decodeB8G8R8A8(const std::byte* source, float* rgba) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(source);
    setPixel(rgba, unorm8(bytes[2]), unorm8(bytes[1]), unorm8(bytes[0]), unorm8(bytes[3]));
}

void decodeR8G8B8A8(const std::byte* source, float* rgba) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(source);
    setPixel(rgba, unorm8(bytes[0]), unorm8(bytes[1]), unorm8(bytes[2]), unorm8(bytes[3]));
}

void decodeR8(const std::byte* source, float* rgba) { setGray(rgba, unorm8(load<std::uint8_t>(source))); }

void decodeA8(const std::byte* source, float* rgba) { setPixel(rgba, 0.0f, 0.0f, 0.0f, unorm8(load<std::uint8_t>(source))); }

void decodeR8G8(const std::byte* source, float* rgba) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(source);
    setPixel(rgba, unorm8(bytes[0]), unorm8(bytes[1]), 0.0f, 1.0f);
}

void decodeR16(const std::byte* source, float* rgba) { setGray(rgba, unorm16(load<std::uint16_t>(source))); }

void decodeR16G16(const std::byte* source, float* rgba) {
    setPixel(rgba, unorm16(load<std::uint16_t>(source)), unorm16(load<std::uint16_t>(source + 2)), 0.0f, 1.0f);
}

void decodeR16G16B16A16(const std::byte* source, float* rgba) {
    setPixel(rgba, unorm16(load<std::uint16_t>(source)), unorm16(load<std::uint16_t>(source + 2)), unorm16(load<std::uint16_t>(source + 4)),
             unorm16(load<std::uint16_t>(source + 6)));
}

void decodeR16F(const std::byte* source, float* rgba) { setGray(rgba, half(load<std::uint16_t>(source))); }

void decodeR16G16F(const std::byte* source, float* rgba) {
    setPixel(rgba, half(load<std::uint16_t>(source)), half(load<std::uint16_t>(source + 2)), 0.0f, 1.0f);
}

void decodeR16G16B16A16F(const std::byte* source, float* rgba) {
    setPixel(rgba, half(load<std::uint16_t>(source)), half(load<std::uint16_t>(source + 2)), half(load<std::uint16_t>(source + 4)),
             half(load<std::uint16_t>(source + 6)));
}

void decodeR32F(const std::byte* source, float* rgba) { setGray(rgba, load<float>(source)); }

void decodeR32G32F(const std::byte* source, float* rgba) { setPixel(rgba, load<float>(source), load<float>(source + 4), 0.0f, 1.0f); }

void decodeR32G32B32F(const std::byte* source, float* rgba) {
    setPixel(rgba, load<float>(source), load<float>(source + 4), load<float>(source + 8), 1.0f);
}

void decodeR32G32B32A32F(const std::byte* source, float* rgba) {
    setPixel(rgba, load<float>(source), load<float>(source + 4), load<float>(source + 8), load<float>(source + 12));
}

void decodeR11G11B10F(const std::byte* source, float* rgba) {
    const auto bits = load<std::uint32_t>(source);
    setPixel(rgba, smallFloat(bits & 0x7ffu, 6), smallFloat((bits >> 11) & 0x7ffu, 6), smallFloat(bits >> 22, 5), 1.0f);
}

void decodeR10G10B10A2(const std::byte* source, float* rgba) {
    const auto bits = load<std::uint32_t>(source);
    setPixel(rgba, unormBits(bits & 0x3ffu, 10), unormBits((bits >> 10) & 0x3ffu, 10), unormBits((bits >> 20) & 0x3ffu, 10), unormBits(bits >> 30, 2));
}

void decodeB5G6R5(const std::byte* source, float* rgba) {
    const auto bits = load<std::uint16_t>(source);
    setPixel(rgba, unormBits(bits >> 11, 5), unormBits((bits >> 5) & 0x3fu, 6), unormBits(bits & 0x1fu, 5), 1.0f);
}

void decodeB5G5R5A1(const std::byte* source, float* rgba) {
    const auto bits = load<std::uint16_t>(source);
    setPixel(rgba, unormBits((bits >> 10) & 0x1fu, 5), unormBits((bits >> 5) & 0x1fu, 5), unormBits(bits & 0x1fu, 5), static_cast<float>(bits >> 15));
}

void decodeR9G9B9E5(const std::byte* source, float* rgba) {
    const auto bits = load<std::uint32_t>(source);
    const float scale = std::ldexp(1.0f, static_cast<int>(bits >> 27) - 15 - 9);
    setPixel(rgba, static_cast<float>(bits & 0x1ffu) * scale, static_cast<float>((bits >> 9) & 0x1ffu) * scale,
             static_cast<float>((bits >> 18) & 0x1ffu) * scale, 1.0f);
}

std::optional<FormatInfo> findFormat(EPixelFormat format) {
    switch (format) {
    case EPixelFormat::PF_DXT1:
        return FormatInfo{4, 8, decodeBc1};
    case EPixelFormat::PF_DXT3:
        return FormatInfo{4, 16, decodeBc2};
    case EPixelFormat::PF_DXT5:
        return FormatInfo{4, 16, decodeBc3};
    case EPixelFormat::PF_BC4:
        return FormatInfo{4, 8, decodeBc4};
    case EPixelFormat::PF_BC5:
        return FormatInfo{4, 16, decodeBc5};
    case EPixelFormat::PF_BC6H:
        return FormatInfo{4, 16, decodeBc6h};
    case EPixelFormat::PF_BC7:
        return FormatInfo{4, 16, decodeBc7};
    case EPixelFormat::PF_B8G8R8A8:
        return FormatInfo{1, 4, decodeB8G8R8A8};
    case EPixelFormat::PF_R8G8B8A8:
        return FormatInfo{1, 4, decodeR8G8B8A8};
    case EPixelFormat::PF_G8:
    case EPixelFormat::PF_L8:
    case EPixelFormat::PF_R8:
        return FormatInfo{1, 1, decodeR8};
    case EPixelFormat::PF_A8:
        return FormatInfo{1, 1, decodeA8};
    case EPixelFormat::PF_R8G8:
        return FormatInfo{1, 2, decodeR8G8};
    case EPixelFormat::PF_G16:
        return FormatInfo{1, 2, decodeR16};
    case EPixelFormat::PF_G16R16:
        return FormatInfo{1, 4, decodeR16G16};
    case EPixelFormat::PF_A16B16G16R16:
    case EPixelFormat::PF_R16G16B16A16_UNORM:
        return FormatInfo{1, 8, decodeR16G16B16A16};
    case EPixelFormat::PF_R16F:
    case EPixelFormat::PF_R16F_FILTER:
        return FormatInfo{1, 2, decodeR16F};
    case EPixelFormat::PF_G16R16F:
    case EPixelFormat::PF_G16R16F_FILTER:
        return FormatInfo{1, 4, decodeR16G16F};
    case EPixelFormat::PF_FloatRGBA:
        return FormatInfo{1, 8, decodeR16G16B16A16F};
    case EPixelFormat::PF_R32_FLOAT:
        return FormatInfo{1, 4, decodeR32F};
    case EPixelFormat::PF_G32R32F:
        return FormatInfo{1, 8, decodeR32G32F};
    case EPixelFormat::PF_R32G32B32F:
        return FormatInfo{1, 12, decodeR32G32B32F};
    case EPixelFormat::PF_A32B32G32R32F:
        return FormatInfo{1, 16, decodeR32G32B32A32F};
    case EPixelFormat::PF_FloatRGB:
    case EPixelFormat::PF_FloatR11G11B10:
        return FormatInfo{1, 4, decodeR11G11B10F};
    case EPixelFormat::PF_A2B10G10R10:
        return FormatInfo{1, 4, decodeR10G10B10A2};
    case EPixelFormat::PF_R5G6B5_UNORM:
        return FormatInfo{1, 2, decodeB5G6R5};
    case EPixelFormat::PF_B5G5R5A1_UNORM:
        return FormatInfo{1, 2, decodeB5G5R5A1};
    case EPixelFormat::PF_R9G9B9EXP5:
        return FormatInfo{1, 4, decodeR9G9B9E5};
    default:
        return std::nullopt;
    }
}

int countBlocks(int size, int blockSize) { return (size + blockSize - 1) / blockSize; }

}

std::size_t PixelLayout::getSliceSize() const {
    const std::optional<FormatInfo> info = findFormat(format);
    if (!info)
        return 0;
    return static_cast<std::size_t>(countBlocks(sizeX, info->blockSize)) * countBlocks(sizeY, info->blockSize) * info->blockBytes;
}

bool isDecodable(EPixelFormat format) { return findFormat(format).has_value(); }

ConversionResult<DecodedImage> decodePixels(const PixelLayout& layout, std::span<const std::byte> data) {
    const std::optional<FormatInfo> info = findFormat(layout.format);
    if (!info)
        return conversionFailure(std::format("the pixel format {} has no decoder", static_cast<int>(layout.format)));
    if (layout.sizeX <= 0 || layout.sizeY <= 0 || layout.slices <= 0)
        return conversionFailure(std::format("the image is {}x{}x{}", layout.sizeX, layout.sizeY, layout.slices));
    if (data.size() < layout.getSize())
        return conversionFailure(std::format("the image needs {} bytes but has {}", layout.getSize(), data.size()));
    DecodedImage image{layout.sizeX, layout.sizeY, layout.slices, {}};
    image.rgba.resize(static_cast<std::size_t>(layout.sizeX) * layout.sizeY * layout.slices * 4);
    const int blocksX = countBlocks(layout.sizeX, info->blockSize);
    const int blocksY = countBlocks(layout.sizeY, info->blockSize);
    const std::byte* source = data.data();
    Block block;
    for (int slice = 0; slice < layout.slices; slice++)
        for (int blockY = 0; blockY < blocksY; blockY++)
            for (int blockX = 0; blockX < blocksX; blockX++, source += info->blockBytes) {
                info->decode(source, block.data());
                const int rows = std::min(info->blockSize, layout.sizeY - blockY * info->blockSize);
                const int columns = std::min(info->blockSize, layout.sizeX - blockX * info->blockSize);
                for (int row = 0; row < rows; row++) {
                    const std::size_t pixel = (static_cast<std::size_t>(slice) * layout.sizeY + blockY * info->blockSize + row) * layout.sizeX + blockX * info->blockSize;
                    std::copy_n(block.data() + row * info->blockSize * 4, columns * 4, image.rgba.data() + pixel * 4);
                }
            }
    return image;
}

}
