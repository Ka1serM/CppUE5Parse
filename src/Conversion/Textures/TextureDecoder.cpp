#include "Conversion/Textures/TextureDecoder.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <memory>
#include <optional>

#define BCDEC_IMPLEMENTATION
#define BCDEC_STATIC
#include <astcenc.h>
#include <bcdec.h>

#include "Conversion/Textures/EtcDecoder.h"
#include "Conversion/Textures/PvrtcDecoder.h"

namespace ue5parse::conversion {

namespace {

using runtime::EPixelFormat;

constexpr int blockPixels = 16;
using Block = std::array<float, blockPixels * 4>;
using DecodeBlock = void (*)(const std::byte* source, float* rgba);

struct AstcMode {
    astcenc_profile profile = ASTCENC_PRF_LDR;
    astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
};

struct FormatInfo {
    int blockSizeX = 1;
    int blockSizeY = 1;
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

using Rgb = std::array<int, 3>;

int expand5(int value) { return value << 3 | value >> 2; }

int expand6(int value) { return value << 2 | value >> 4; }

Rgb mixAtcColors(const Rgb& first, const Rgb& second, int firstWeight) {
    Rgb mixed;
    for (int c = 0; c < 3; c++)
        mixed[c] = (first[c] * firstWeight + second[c] * (8 - firstWeight)) / 8;
    return mixed;
}

//! An ATC block whose first color sets bit 15 holds black, the first color minus a quarter of the second, the first color and the second color.
void decodeAtcColorBytes(const std::byte* source, std::span<std::uint8_t, blockPixels * 4> bytes) {
    const auto first = load<std::uint16_t>(source);
    const auto second = load<std::uint16_t>(source + 2);
    const auto indices = load<std::uint32_t>(source + 4);
    const Rgb color0{expand5((first >> 10) & 31), expand5((first >> 5) & 31), expand5(first & 31)};
    const Rgb color3{expand5((second >> 11) & 31), expand6((second >> 5) & 63), expand5(second & 31)};
    std::array<Rgb, 4> colors{color0, mixAtcColors(color0, color3, 5), mixAtcColors(color0, color3, 3), color3};
    if ((first & 0x8000) != 0) {
        colors = {Rgb{}, color0, color0, color3};
        for (int c = 0; c < 3; c++)
            colors[1][c] = std::max(color0[c] - color3[c] / 4, 0);
    }
    for (int i = 0; i < blockPixels; i++) {
        const Rgb& color = colors[(indices >> (i * 2)) & 3];
        bytes[i * 4] = static_cast<std::uint8_t>(color[0]);
        bytes[i * 4 + 1] = static_cast<std::uint8_t>(color[1]);
        bytes[i * 4 + 2] = static_cast<std::uint8_t>(color[2]);
        bytes[i * 4 + 3] = 255;
    }
}

void decodeAtcRgb(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 4> bytes;
    decodeAtcColorBytes(source, bytes);
    decodeBytes(bytes, rgba);
}

void decodeAtcRgbaExplicit(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 4> bytes;
    decodeAtcColorBytes(source + 8, bytes);
    bcdec__sharp_alpha_block(source, bytes.data() + 3, 16);
    decodeBytes(bytes, rgba);
}

void decodeAtcRgbaInterpolated(const std::byte* source, float* rgba) {
    std::array<std::uint8_t, blockPixels * 4> bytes;
    decodeAtcColorBytes(source + 8, bytes);
    bcdec__smooth_alpha_block(source, bytes.data() + 3, 16, 4);
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

//! UE cooks PF_V8U8 as the red and green bytes minus 128.
void decodeV8U8(const std::byte* source, float* rgba) {
    setPixel(rgba, unorm8(static_cast<std::uint8_t>(load<std::int8_t>(source) + 128)), unorm8(static_cast<std::uint8_t>(load<std::int8_t>(source + 1) + 128)), 0.0f, 1.0f);
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

//! UE decodes HDR ASTC with opaque alpha, as BC6H has none.
constexpr AstcMode astcHdr{ASTCENC_PRF_HDR_RGB_LDR_A, {ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_1}};
//! UE encodes the NORM_RG normal maps as luminance and alpha and samples them as red and green.
constexpr AstcMode astcNormalRg{ASTCENC_PRF_LDR, {ASTCENC_SWZ_R, ASTCENC_SWZ_A, ASTCENC_SWZ_0, ASTCENC_SWZ_1}};

std::optional<AstcMode> findAstcMode(EPixelFormat format) {
    switch (format) {
    case EPixelFormat::PF_ASTC_4x4:
    case EPixelFormat::PF_ASTC_6x6:
    case EPixelFormat::PF_ASTC_8x8:
    case EPixelFormat::PF_ASTC_10x10:
    case EPixelFormat::PF_ASTC_12x12:
        return AstcMode{};
    case EPixelFormat::PF_ASTC_4x4_HDR:
    case EPixelFormat::PF_ASTC_6x6_HDR:
    case EPixelFormat::PF_ASTC_8x8_HDR:
    case EPixelFormat::PF_ASTC_10x10_HDR:
    case EPixelFormat::PF_ASTC_12x12_HDR:
        return astcHdr;
    case EPixelFormat::PF_ASTC_4x4_NORM_RG:
    case EPixelFormat::PF_ASTC_6x6_NORM_RG:
    case EPixelFormat::PF_ASTC_8x8_NORM_RG:
    case EPixelFormat::PF_ASTC_10x10_NORM_RG:
    case EPixelFormat::PF_ASTC_12x12_NORM_RG:
        return astcNormalRg;
    default:
        return std::nullopt;
    }
}

std::optional<FormatInfo> findFormat(EPixelFormat format) {
    switch (format) {
    case EPixelFormat::PF_DXT1:
        return FormatInfo{4, 4, 8, decodeBc1};
    case EPixelFormat::PF_DXT3:
        return FormatInfo{4, 4, 16, decodeBc2};
    case EPixelFormat::PF_DXT5:
        return FormatInfo{4, 4, 16, decodeBc3};
    case EPixelFormat::PF_BC4:
        return FormatInfo{4, 4, 8, decodeBc4};
    case EPixelFormat::PF_BC5:
        return FormatInfo{4, 4, 16, decodeBc5};
    case EPixelFormat::PF_BC6H:
        return FormatInfo{4, 4, 16, decodeBc6h};
    case EPixelFormat::PF_BC7:
        return FormatInfo{4, 4, 16, decodeBc7};
    case EPixelFormat::PF_PVRTC2:
        return FormatInfo{8, 4, 8, nullptr};
    case EPixelFormat::PF_PVRTC4:
        return FormatInfo{4, 4, 8, nullptr};
    case EPixelFormat::PF_ATC_RGB:
        return FormatInfo{4, 4, 8, decodeAtcRgb};
    case EPixelFormat::PF_ATC_RGBA_E:
        return FormatInfo{4, 4, 16, decodeAtcRgbaExplicit};
    case EPixelFormat::PF_ATC_RGBA_I:
        return FormatInfo{4, 4, 16, decodeAtcRgbaInterpolated};
    case EPixelFormat::PF_ETC1:
    case EPixelFormat::PF_ETC2_RGB:
        return FormatInfo{4, 4, 8, decodeEtc2Rgb};
    case EPixelFormat::PF_ETC2_RGBA:
        return FormatInfo{4, 4, 16, decodeEtc2Rgba};
    case EPixelFormat::PF_ETC2_R11_EAC:
        return FormatInfo{4, 4, 8, decodeEacR11};
    case EPixelFormat::PF_ETC2_RG11_EAC:
        return FormatInfo{4, 4, 16, decodeEacRg11};
    case EPixelFormat::PF_ASTC_4x4:
    case EPixelFormat::PF_ASTC_4x4_HDR:
    case EPixelFormat::PF_ASTC_4x4_NORM_RG:
        return FormatInfo{4, 4, 16, nullptr};
    case EPixelFormat::PF_ASTC_6x6:
    case EPixelFormat::PF_ASTC_6x6_HDR:
    case EPixelFormat::PF_ASTC_6x6_NORM_RG:
        return FormatInfo{6, 6, 16, nullptr};
    case EPixelFormat::PF_ASTC_8x8:
    case EPixelFormat::PF_ASTC_8x8_HDR:
    case EPixelFormat::PF_ASTC_8x8_NORM_RG:
        return FormatInfo{8, 8, 16, nullptr};
    case EPixelFormat::PF_ASTC_10x10:
    case EPixelFormat::PF_ASTC_10x10_HDR:
    case EPixelFormat::PF_ASTC_10x10_NORM_RG:
        return FormatInfo{10, 10, 16, nullptr};
    case EPixelFormat::PF_ASTC_12x12:
    case EPixelFormat::PF_ASTC_12x12_HDR:
    case EPixelFormat::PF_ASTC_12x12_NORM_RG:
        return FormatInfo{12, 12, 16, nullptr};
    case EPixelFormat::PF_B8G8R8A8:
        return FormatInfo{1, 1, 4, decodeB8G8R8A8};
    case EPixelFormat::PF_R8G8B8A8:
        return FormatInfo{1, 1, 4, decodeR8G8B8A8};
    case EPixelFormat::PF_G8:
    case EPixelFormat::PF_L8:
    case EPixelFormat::PF_R8:
        return FormatInfo{1, 1, 1, decodeR8};
    case EPixelFormat::PF_A8:
        return FormatInfo{1, 1, 1, decodeA8};
    case EPixelFormat::PF_R8G8:
        return FormatInfo{1, 1, 2, decodeR8G8};
    case EPixelFormat::PF_V8U8:
        return FormatInfo{1, 1, 2, decodeV8U8};
    case EPixelFormat::PF_G16:
        return FormatInfo{1, 1, 2, decodeR16};
    case EPixelFormat::PF_G16R16:
        return FormatInfo{1, 1, 4, decodeR16G16};
    case EPixelFormat::PF_A16B16G16R16:
    case EPixelFormat::PF_R16G16B16A16_UNORM:
        return FormatInfo{1, 1, 8, decodeR16G16B16A16};
    case EPixelFormat::PF_R16F:
    case EPixelFormat::PF_R16F_FILTER:
        return FormatInfo{1, 1, 2, decodeR16F};
    case EPixelFormat::PF_G16R16F:
    case EPixelFormat::PF_G16R16F_FILTER:
        return FormatInfo{1, 1, 4, decodeR16G16F};
    case EPixelFormat::PF_FloatRGBA:
        return FormatInfo{1, 1, 8, decodeR16G16B16A16F};
    case EPixelFormat::PF_R32_FLOAT:
        return FormatInfo{1, 1, 4, decodeR32F};
    case EPixelFormat::PF_G32R32F:
        return FormatInfo{1, 1, 8, decodeR32G32F};
    case EPixelFormat::PF_R32G32B32F:
        return FormatInfo{1, 1, 12, decodeR32G32B32F};
    case EPixelFormat::PF_A32B32G32R32F:
        return FormatInfo{1, 1, 16, decodeR32G32B32A32F};
    case EPixelFormat::PF_FloatRGB:
    case EPixelFormat::PF_FloatR11G11B10:
        return FormatInfo{1, 1, 4, decodeR11G11B10F};
    case EPixelFormat::PF_A2B10G10R10:
        return FormatInfo{1, 1, 4, decodeR10G10B10A2};
    case EPixelFormat::PF_R5G6B5_UNORM:
        return FormatInfo{1, 1, 2, decodeB5G6R5};
    case EPixelFormat::PF_B5G5R5A1_UNORM:
        return FormatInfo{1, 1, 2, decodeB5G5R5A1};
    case EPixelFormat::PF_R9G9B9EXP5:
        return FormatInfo{1, 1, 4, decodeR9G9B9E5};
    default:
        return std::nullopt;
    }
}

bool isPvrtc(EPixelFormat format) { return format == EPixelFormat::PF_PVRTC2 || format == EPixelFormat::PF_PVRTC4; }

//! UE stores at least 2x2 blocks per PVRTC mip.
int countBlocks(EPixelFormat format, int size, int blockSize) {
    const int blocks = (size + blockSize - 1) / blockSize;
    return isPvrtc(format) ? std::max(blocks, 2) : blocks;
}

std::optional<std::string> decodeAstc(const PixelLayout& layout, const FormatInfo& info, const AstcMode& mode, std::span<const std::byte> data, DecodedImage& image) {
    astcenc_config config;
    astcenc_error status = astcenc_config_init(mode.profile, info.blockSizeX, info.blockSizeY, 1, ASTCENC_PRE_FASTEST, ASTCENC_FLG_DECOMPRESS_ONLY, &config);
    if (status != ASTCENC_SUCCESS)
        return astcenc_get_error_string(status);
    astcenc_context* allocated = nullptr;
    status = astcenc_context_alloc(&config, 1, &allocated, nullptr);
    if (status != ASTCENC_SUCCESS)
        return astcenc_get_error_string(status);
    const std::unique_ptr<astcenc_context, decltype(&astcenc_context_free)> context(allocated, astcenc_context_free);
    const std::size_t sliceSize = layout.getSliceSize();
    const std::size_t slicePixels = static_cast<std::size_t>(layout.sizeX) * layout.sizeY;
    for (int slice = 0; slice < layout.slices; slice++) {
        void* pixels = image.rgba.data() + slice * slicePixels * 4;
        astcenc_image target{static_cast<unsigned>(layout.sizeX), static_cast<unsigned>(layout.sizeY), 1, ASTCENC_TYPE_F32, &pixels};
        const auto* source = reinterpret_cast<const std::uint8_t*>(data.data()) + slice * sliceSize;
        status = astcenc_decompress_image(context.get(), source, sliceSize, &target, &mode.swizzle, 0);
        if (status != ASTCENC_SUCCESS)
            return astcenc_get_error_string(status);
        astcenc_decompress_reset(context.get());
    }
    return std::nullopt;
}

}

std::size_t PixelLayout::getSliceSize() const {
    const std::optional<FormatInfo> info = findFormat(format);
    if (!info)
        return 0;
    return static_cast<std::size_t>(countBlocks(format, sizeX, info->blockSizeX)) * countBlocks(format, sizeY, info->blockSizeY) * info->blockBytes;
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
    if (const std::optional<AstcMode> mode = findAstcMode(layout.format)) {
        const std::optional<std::string> error = decodeAstc(layout, *info, *mode, data, image);
        if (error)
            return conversionFailure(*error);
        return image;
    }
    const int blocksX = countBlocks(layout.format, layout.sizeX, info->blockSizeX);
    const int blocksY = countBlocks(layout.format, layout.sizeY, info->blockSizeY);
    if (isPvrtc(layout.format)) {
        if (!std::has_single_bit(static_cast<unsigned>(blocksX)) || !std::has_single_bit(static_cast<unsigned>(blocksY)))
            return conversionFailure(std::format("a PVRTC image of {}x{} blocks is not a power of two", blocksX, blocksY));
        const std::size_t slicePixels = static_cast<std::size_t>(layout.sizeX) * layout.sizeY;
        for (int slice = 0; slice < layout.slices; slice++)
            decodePvrtc(data.data() + slice * layout.getSliceSize(), blocksX, blocksY, layout.format == EPixelFormat::PF_PVRTC2, layout.sizeX, layout.sizeY,
                        image.rgba.data() + slice * slicePixels * 4);
        return image;
    }
    const std::byte* source = data.data();
    Block block;
    for (int slice = 0; slice < layout.slices; slice++)
        for (int blockY = 0; blockY < blocksY; blockY++)
            for (int blockX = 0; blockX < blocksX; blockX++, source += info->blockBytes) {
                info->decode(source, block.data());
                const int rows = std::min(info->blockSizeY, layout.sizeY - blockY * info->blockSizeY);
                const int columns = std::min(info->blockSizeX, layout.sizeX - blockX * info->blockSizeX);
                for (int row = 0; row < rows; row++) {
                    const std::size_t pixel = (static_cast<std::size_t>(slice) * layout.sizeY + blockY * info->blockSizeY + row) * layout.sizeX + blockX * info->blockSizeX;
                    std::copy_n(block.data() + row * info->blockSizeX * 4, columns * 4, image.rgba.data() + pixel * 4);
                }
            }
    return image;
}

}
