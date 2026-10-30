#include "ue5parse/runtime/Core/PixelFormat.h"

namespace ue5parse::runtime {

namespace {

struct PixelFormatName {
    EPixelFormat format;
    const char* name;
};

constexpr PixelFormatName pixelFormatNames[] = {
    {EPixelFormat::PF_Unknown, "PF_Unknown"},
    {EPixelFormat::PF_A32B32G32R32F, "PF_A32B32G32R32F"},
    {EPixelFormat::PF_B8G8R8A8, "PF_B8G8R8A8"},
    {EPixelFormat::PF_G8, "PF_G8"},
    {EPixelFormat::PF_G16, "PF_G16"},
    {EPixelFormat::PF_DXT1, "PF_DXT1"},
    {EPixelFormat::PF_DXT3, "PF_DXT3"},
    {EPixelFormat::PF_DXT5, "PF_DXT5"},
    {EPixelFormat::PF_UYVY, "PF_UYVY"},
    {EPixelFormat::PF_FloatRGB, "PF_FloatRGB"},
    {EPixelFormat::PF_FloatRGBA, "PF_FloatRGBA"},
    {EPixelFormat::PF_DepthStencil, "PF_DepthStencil"},
    {EPixelFormat::PF_ShadowDepth, "PF_ShadowDepth"},
    {EPixelFormat::PF_R32_FLOAT, "PF_R32_FLOAT"},
    {EPixelFormat::PF_G16R16, "PF_G16R16"},
    {EPixelFormat::PF_G16R16F, "PF_G16R16F"},
    {EPixelFormat::PF_G16R16F_FILTER, "PF_G16R16F_FILTER"},
    {EPixelFormat::PF_G32R32F, "PF_G32R32F"},
    {EPixelFormat::PF_A2B10G10R10, "PF_A2B10G10R10"},
    {EPixelFormat::PF_A16B16G16R16, "PF_A16B16G16R16"},
    {EPixelFormat::PF_D24, "PF_D24"},
    {EPixelFormat::PF_R16F, "PF_R16F"},
    {EPixelFormat::PF_R16F_FILTER, "PF_R16F_FILTER"},
    {EPixelFormat::PF_BC5, "PF_BC5"},
    {EPixelFormat::PF_V8U8, "PF_V8U8"},
    {EPixelFormat::PF_A1, "PF_A1"},
    {EPixelFormat::PF_FloatR11G11B10, "PF_FloatR11G11B10"},
    {EPixelFormat::PF_A8, "PF_A8"},
    {EPixelFormat::PF_R32_UINT, "PF_R32_UINT"},
    {EPixelFormat::PF_R32_SINT, "PF_R32_SINT"},
    {EPixelFormat::PF_PVRTC2, "PF_PVRTC2"},
    {EPixelFormat::PF_PVRTC4, "PF_PVRTC4"},
    {EPixelFormat::PF_R16_UINT, "PF_R16_UINT"},
    {EPixelFormat::PF_R16_SINT, "PF_R16_SINT"},
    {EPixelFormat::PF_R16G16B16A16_UINT, "PF_R16G16B16A16_UINT"},
    {EPixelFormat::PF_R16G16B16A16_SINT, "PF_R16G16B16A16_SINT"},
    {EPixelFormat::PF_R5G6B5_UNORM, "PF_R5G6B5_UNORM"},
    {EPixelFormat::PF_R8G8B8A8, "PF_R8G8B8A8"},
    {EPixelFormat::PF_A8R8G8B8, "PF_A8R8G8B8"},
    {EPixelFormat::PF_BC4, "PF_BC4"},
    {EPixelFormat::PF_R8G8, "PF_R8G8"},
    {EPixelFormat::PF_ATC_RGB, "PF_ATC_RGB"},
    {EPixelFormat::PF_ATC_RGBA_E, "PF_ATC_RGBA_E"},
    {EPixelFormat::PF_ATC_RGBA_I, "PF_ATC_RGBA_I"},
    {EPixelFormat::PF_X24_G8, "PF_X24_G8"},
    {EPixelFormat::PF_ETC1, "PF_ETC1"},
    {EPixelFormat::PF_ETC2_RGB, "PF_ETC2_RGB"},
    {EPixelFormat::PF_ETC2_RGBA, "PF_ETC2_RGBA"},
    {EPixelFormat::PF_R32G32B32A32_UINT, "PF_R32G32B32A32_UINT"},
    {EPixelFormat::PF_R16G16_UINT, "PF_R16G16_UINT"},
    {EPixelFormat::PF_ASTC_4x4, "PF_ASTC_4x4"},
    {EPixelFormat::PF_ASTC_6x6, "PF_ASTC_6x6"},
    {EPixelFormat::PF_ASTC_8x8, "PF_ASTC_8x8"},
    {EPixelFormat::PF_ASTC_10x10, "PF_ASTC_10x10"},
    {EPixelFormat::PF_ASTC_12x12, "PF_ASTC_12x12"},
    {EPixelFormat::PF_BC6H, "PF_BC6H"},
    {EPixelFormat::PF_BC7, "PF_BC7"},
    {EPixelFormat::PF_R8_UINT, "PF_R8_UINT"},
    {EPixelFormat::PF_L8, "PF_L8"},
    {EPixelFormat::PF_XGXR8, "PF_XGXR8"},
    {EPixelFormat::PF_R8G8B8A8_UINT, "PF_R8G8B8A8_UINT"},
    {EPixelFormat::PF_R8G8B8A8_SNORM, "PF_R8G8B8A8_SNORM"},
    {EPixelFormat::PF_R16G16B16A16_UNORM, "PF_R16G16B16A16_UNORM"},
    {EPixelFormat::PF_R16G16B16A16_SNORM, "PF_R16G16B16A16_SNORM"},
    {EPixelFormat::PF_PLATFORM_HDR_0, "PF_PLATFORM_HDR_0"},
    {EPixelFormat::PF_PLATFORM_HDR_1, "PF_PLATFORM_HDR_1"},
    {EPixelFormat::PF_PLATFORM_HDR_2, "PF_PLATFORM_HDR_2"},
    {EPixelFormat::PF_NV12, "PF_NV12"},
    {EPixelFormat::PF_R32G32_UINT, "PF_R32G32_UINT"},
    {EPixelFormat::PF_ETC2_R11_EAC, "PF_ETC2_R11_EAC"},
    {EPixelFormat::PF_ETC2_RG11_EAC, "PF_ETC2_RG11_EAC"},
    {EPixelFormat::PF_R8, "PF_R8"},
    {EPixelFormat::PF_B5G5R5A1_UNORM, "PF_B5G5R5A1_UNORM"},
    {EPixelFormat::PF_ASTC_4x4_HDR, "PF_ASTC_4x4_HDR"},
    {EPixelFormat::PF_ASTC_6x6_HDR, "PF_ASTC_6x6_HDR"},
    {EPixelFormat::PF_ASTC_8x8_HDR, "PF_ASTC_8x8_HDR"},
    {EPixelFormat::PF_ASTC_10x10_HDR, "PF_ASTC_10x10_HDR"},
    {EPixelFormat::PF_ASTC_12x12_HDR, "PF_ASTC_12x12_HDR"},
    {EPixelFormat::PF_G16R16_SNORM, "PF_G16R16_SNORM"},
    {EPixelFormat::PF_R8G8_UINT, "PF_R8G8_UINT"},
    {EPixelFormat::PF_R32G32B32_UINT, "PF_R32G32B32_UINT"},
    {EPixelFormat::PF_R32G32B32_SINT, "PF_R32G32B32_SINT"},
    {EPixelFormat::PF_R32G32B32F, "PF_R32G32B32F"},
    {EPixelFormat::PF_R8_SINT, "PF_R8_SINT"},
    {EPixelFormat::PF_R64_UINT, "PF_R64_UINT"},
    {EPixelFormat::PF_R9G9B9EXP5, "PF_R9G9B9EXP5"},
    {EPixelFormat::PF_P010, "PF_P010"},
    {EPixelFormat::PF_ASTC_4x4_NORM_RG, "PF_ASTC_4x4_NORM_RG"},
    {EPixelFormat::PF_ASTC_6x6_NORM_RG, "PF_ASTC_6x6_NORM_RG"},
    {EPixelFormat::PF_ASTC_8x8_NORM_RG, "PF_ASTC_8x8_NORM_RG"},
    {EPixelFormat::PF_ASTC_10x10_NORM_RG, "PF_ASTC_10x10_NORM_RG"},
    {EPixelFormat::PF_ASTC_12x12_NORM_RG, "PF_ASTC_12x12_NORM_RG"},
    {EPixelFormat::PF_R16G16_SINT, "PF_R16G16_SINT"},
    {EPixelFormat::PF_R8G8B8, "PF_R8G8B8"},
    {EPixelFormat::PF_MAX, "PF_MAX"},
    {EPixelFormat::PF_B4G4R4A4, "PF_B4G4R4A4"},
    {EPixelFormat::PF_BC6H_Signed, "PF_BC6H_Signed"},
    {EPixelFormat::PF_ETC2_R11, "PF_ETC2_R11"},
    {EPixelFormat::PF_ASTC_8x5, "PF_ASTC_8x5"},
    {EPixelFormat::PF_ASTC_8x6, "PF_ASTC_8x6"},
    {EPixelFormat::PF_ASTC_10x8, "PF_ASTC_10x8"},
};

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); i++) {
        char x = a[i] >= 'A' && a[i] <= 'Z' ? static_cast<char>(a[i] - 'A' + 'a') : a[i];
        char y = b[i] >= 'A' && b[i] <= 'Z' ? static_cast<char>(b[i] - 'A' + 'a') : b[i];
        if (x != y) return false;
    }
    return true;
}

}

EPixelFormat GetPixelFormatFromString(std::string_view name) {
    for (const PixelFormatName& entry : pixelFormatNames)
        if (equalsIgnoreCase(name, entry.name)) return entry.format;
    return EPixelFormat::PF_Unknown;
}

const char* GetPixelFormatString(EPixelFormat format) {
    for (const PixelFormatName& entry : pixelFormatNames)
        if (entry.format == format) return entry.name;
    return "PF_Unknown";
}

}
