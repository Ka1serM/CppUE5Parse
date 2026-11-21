#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <format>
#include <optional>
#include <span>

#include "AssetSuite.h"
#include "Conversion/Textures/TextureDecoder.h"
#include "Conversion/Textures/VirtualTextureDecoder.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectHash.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureCube.h"

namespace test::assets {

namespace {

namespace conversion = ue5parse::conversion;

using Rgba = std::array<float, 4>;

void checkSize(const std::string& path, const Json& value) {
    rt::UTexture2D* texture = loadObject<rt::UTexture2D>(path);
    if (texture == nullptr)
        return;
    double size[] = {static_cast<double>(texture->GetSizeX()), static_cast<double>(texture->GetSizeY())};
    checkNumbers(size, value, 0.0, describe(path, "size"));
}

void checkSettings(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "CompressionSettings", value.getArray()[0]);
    checkReflectedProperty(path, "SRGB", value.getArray()[1]);
}

struct Shape {
    int sizeX = 0;
    int sizeY = 0;
    int seed = 0;
};

using Generator = Rgba (*)(const Shape& shape, int x, int y, int slice);

struct Source {
    std::string_view name;
    Generator generate;
    int seed;
};

float byte(int value) { return static_cast<float>(static_cast<std::uint8_t>(value)) / 255.0f; }

bool isChecked(const Shape& shape, int x, int y, int slice) {
    return ((x * 8) / std::max(shape.sizeX, 1) + (y * 8) / std::max(shape.sizeY, 1) + slice + shape.seed) % 2 == 0;
}

Rgba rawColor(const Shape& shape, int x, int y, int slice) {
    return {byte(y * 255 / std::max(shape.sizeY - 1, 1)), byte(x * 255 / std::max(shape.sizeX - 1, 1)), byte(isChecked(shape, x, y, slice) ? 200 : 30),
            byte(255 - slice * 40)};
}

Rgba rawHdr(const Shape& shape, int x, int y, int slice) {
    return {static_cast<float>(x) * 4.0f / static_cast<float>(shape.sizeX), static_cast<float>(y) * 4.0f / static_cast<float>(shape.sizeY),
            isChecked(shape, x, y, slice) ? 2.0f : 0.25f, 1.0f};
}

Rgba gray(float value) { return {value, value, value, 1.0f}; }

Rgba rawGray(const Shape&, int x, int y, int slice) { return gray(byte((x ^ y ^ (slice * 31)) & 0xff)); }

Rgba albedo(const Shape&, int x, int y, int) { return {byte((x / 32 + y / 32) % 2 == 0 ? 220 : 40), byte(x), byte(y), 1.0f}; }

int unitToByte(float value) { return static_cast<int>((value * 0.5f + 0.5f) * 255.0f); }

Rgba normal(const Shape&, int x, int y, int) {
    const float normalX = std::sin(static_cast<float>(x) * 0.1f) * 0.4f;
    const float normalY = std::cos(static_cast<float>(y) * 0.1f) * 0.4f;
    const float normalZ = std::sqrt(std::max(0.0f, 1.0f - normalX * normalX - normalY * normalY));
    return {byte(unitToByte(normalX)), byte(unitToByte(normalY)), byte(unitToByte(normalZ)), 1.0f};
}

Rgba mask(const Shape&, int x, int y, int) { return gray(byte((x ^ y) & 0xff)); }

Rgba hdr(const Shape&, int x, int y, int) { return {static_cast<float>(x) / 8.0f, static_cast<float>(y) / 8.0f, 4.0f, 1.0f}; }

Rgba subUv(const Shape&, int x, int y, int) {
    const int cell = (y / 128) * 2 + x / 128;
    const float distanceX = static_cast<float>(x % 128) - 64.0f;
    const float distanceY = static_cast<float>(y % 128) - 64.0f;
    const float radius = 20.0f + static_cast<float>(cell) * 10.0f;
    return {1.0f, byte(64 * cell), byte(32), distanceX * distanceX + distanceY * distanceY < radius * radius ? 1.0f : 0.0f};
}

Rgba cube(const Shape&, int x, int y, int face) { return {byte(40 * face), byte(x * 4), byte(y * 4), 1.0f}; }

constexpr int udimBlockSize = 256;

Rgba udim(const Shape&, int x, int y, int) {
    const int blockX = x / udimBlockSize;
    const int blockY = y / udimBlockSize;
    if (blockX == 1 && blockY == 1)
        return {0.0f, 0.0f, 0.0f, 0.0f};
    return {byte(y % udimBlockSize), byte(x % udimBlockSize), byte((blockX + blockY * 2) * 100), 1.0f};
}

//! The pixels and checker seeds that tools/RefGen gives each texture (RefGenMaterials.cpp, RefGenCatalog.cpp).
constexpr Source sources[] = {
    {"T_NoMips", rawColor, 1},  {"T_NonPow2", rawColor, 2}, {"T_Large", rawColor, 3},    {"T_NeverStream", rawColor, 4}, {"T_G8", rawGray, 5},
    {"T_Virtual", rawColor, 6}, {"T_Array", rawColor, 7},   {"T_Volume", rawColor, 8},   {"T_CubeArray", rawColor, 9},   {"T_CubeHdr", rawHdr, 10},
    {"T_Albedo", albedo, 0},    {"T_Normal", normal, 0},    {"T_Mask", mask, 0},         {"T_Hdr", hdr, 0},              {"T_SubUV", subUv, 0},
    {"T_Cube", cube, 0},        {"T_Udim", udim, 0},
};

//! UE 5.0 inserts TC_LQ before TC_EncodedReflectionCapture, which UE 4.26 calls TC_ReflectionCapture.
int getEngineValue(rt::TextureCompressionSettings settings, ue5parse::EGame game) {
    if (settings == rt::TC_EncodedReflectionCapture && game < ue5parse::EGame::GAME_UE5_0)
        return rt::TC_LQ;
    return settings;
}

std::optional<Source> findSource(std::string_view name, const rt::UTexture& texture, ue5parse::EGame game) {
    if (name.starts_with("T_TC_")) {
        const bool isFloat = name.contains("HDR") || name.contains("Float") || name.contains("Reflection");
        return Source{name, isFloat ? rawHdr : rawColor, getEngineValue(texture.CompressionSettings, game)};
    }
    for (const Source& source : sources)
        if (source.name == name)
            return source;
    return std::nullopt;
}

struct ChannelPair {
    int decoded;
    int source;
};

constexpr ChannelPair rgba[] = {{0, 0}, {1, 1}, {2, 2}, {3, 3}};
constexpr ChannelPair rgb[] = {{0, 0}, {1, 1}, {2, 2}};
constexpr ChannelPair rg[] = {{0, 0}, {1, 1}};
constexpr ChannelPair red[] = {{0, 0}};
constexpr ChannelPair alpha[] = {{0, 3}};

std::span<const ChannelPair> findChannels(rt::EPixelFormat format, rt::TextureCompressionSettings settings) {
    using rt::EPixelFormat;
    switch (format) {
    case EPixelFormat::PF_G8:
    case EPixelFormat::PF_BC4:
    case EPixelFormat::PF_R16F:
    case EPixelFormat::PF_R32_FLOAT:
    case EPixelFormat::PF_G16:
        return settings == rt::TC_Displacementmap || settings == rt::TC_DistanceFieldFont ? std::span<const ChannelPair>(alpha) : std::span<const ChannelPair>(red);
    case EPixelFormat::PF_BC5:
        return rg;
    case EPixelFormat::PF_DXT1:
    case EPixelFormat::PF_BC6H:
    case EPixelFormat::PF_FloatRGB:
        return rgb;
    default:
        return rgba;
    }
}

struct PixelError {
    double mean = 0.0;
    double max = 0.0;
};

struct MipScale {
    int xy = 1;
    int z = 1;
};

Rgba averageSource(const Source& source, const Shape& shape, const MipScale& scale, int x, int y, int slice) {
    Rgba sum{};
    for (int z = slice * scale.z; z < (slice + 1) * scale.z; z++)
        for (int sourceY = y * scale.xy; sourceY < (y + 1) * scale.xy; sourceY++)
            for (int sourceX = x * scale.xy; sourceX < (x + 1) * scale.xy; sourceX++) {
                const Rgba pixel = source.generate(shape, sourceX, sourceY, z);
                for (std::size_t channel = 0; channel < sum.size(); channel++)
                    sum[channel] += pixel[channel];
            }
    const float count = static_cast<float>(scale.xy * scale.xy * scale.z);
    for (float& channel : sum)
        channel /= count;
    return sum;
}

struct Comparison {
    std::span<const ChannelPair> channels;
    int border = 0;
    PixelError tolerance;
};

PixelError measureError(const conversion::DecodedImage& image, const Source& source, const Shape& shape, const MipScale& scale, const Comparison& comparison) {
    double sum = 0.0;
    double max = 0.0;
    std::size_t count = 0;
    for (int slice = 0; slice < image.slices; slice++)
        for (int y = comparison.border; y < image.sizeY - comparison.border; y++)
            for (int x = comparison.border; x < image.sizeX - comparison.border; x++) {
                const Rgba expected = averageSource(source, shape, scale, x, y, slice);
                const std::span<const float, 4> actual = image.getPixel(x, y, slice);
                for (const ChannelPair& channel : comparison.channels) {
                    const double error = std::abs(actual[channel.decoded] - expected[channel.source]) / std::max(1.0f, std::abs(expected[channel.source]));
                    sum += error;
                    count++;
                    max = std::max(max, error);
                }
            }
    return {sum / static_cast<double>(std::max<std::size_t>(count, 1)), max};
}


PixelError findTolerance(rt::EPixelFormat format) {
    using rt::EPixelFormat;
    switch (format) {
    case EPixelFormat::PF_BC6H:
        return {0.02, 0.3};
    case EPixelFormat::PF_DXT1:
    case EPixelFormat::PF_DXT3:
    case EPixelFormat::PF_DXT5:
    case EPixelFormat::PF_BC4:
    case EPixelFormat::PF_BC5:
    case EPixelFormat::PF_BC7:
        return {0.02, 0.1};
    default:
        return {0.01, 0.04};
    }
}

//! UE 4.26 and older cook the top mip of a cube map through the angular filter, which blurs each face into its neighbors and clears the alpha.
Comparison findComparison(const Cook& cook, const rt::UTexture& texture, rt::EPixelFormat format) {
    if (cook.game < ue5parse::EGame::GAME_UE4_27 && rt::Cast<rt::UTextureCube>(&texture) != nullptr)
        return {rgb, 1, {0.03, 1.0}};
    return {findChannels(format, texture.CompressionSettings), 0, findTolerance(format)};
}

int countSlices(const rt::FTexturePlatformData& data, const rt::FTexture2DMipMap& mip) { return mip.SizeZ > 1 ? mip.SizeZ : data.GetNumSlices(); }

struct TexturePixels {
    conversion::DecodedImage image;
    rt::EPixelFormat format = rt::EPixelFormat::PF_Unknown;
    int mip = 0;
    int sizeX = 0;
    int sizeY = 0;
    MipScale scale;
};

std::optional<conversion::DecodedImage> decodeMip(const std::string& path, const rt::FTexturePlatformData& data, int index) {
    const rt::FTexture2DMipMap& mip = data.Mips[index];
    const conversion::PixelLayout layout{data.PixelFormat, mip.SizeX, mip.SizeY, countSlices(data, mip)};
    if (!CHECK(mip.BulkData.GetPayloadSize() == layout.getSize()))
        return std::nullopt;
    const ue5parse::Result<std::vector<std::byte>> bytes = mip.BulkData.Read();
    if (!bytes) {
        printError(path, bytes.error());
        return std::nullopt;
    }
    conversion::ConversionResult<conversion::DecodedImage> image = conversion::decodePixels(layout, *bytes);
    if (!image) {
        fail(path, image.error().message);
        return std::nullopt;
    }
    return std::move(*image);
}

std::optional<TexturePixels> readMipPixels(const std::string& path, const rt::FTexturePlatformData& data, int first) {
    std::optional<conversion::DecodedImage> image = decodeMip(path, data, first);
    if (!image)
        return std::nullopt;
    const rt::FTexture2DMipMap& top = data.Mips[0];
    const rt::FTexture2DMipMap& mip = data.Mips[first];
    return TexturePixels{std::move(*image), data.PixelFormat, first, top.SizeX, top.SizeY, {top.SizeX / mip.SizeX, top.SizeZ / mip.SizeZ}};
}

std::optional<TexturePixels> readVirtualPixels(const std::string& path, const rt::FVirtualTextureBuiltData& data) {
    conversion::ConversionResult<conversion::DecodedImage> image = conversion::decodeVirtualTexture(data, 0, 0);
    if (!image) {
        if (image.error().cause)
            printError(path, *image.error().cause);
        else
            fail(path, image.error().message);
        return std::nullopt;
    }
    return TexturePixels{std::move(*image), data.LayerTypes[0], 0, static_cast<int>(data.Width), static_cast<int>(data.Height), {}};
}

std::optional<TexturePixels> readPixels(const std::string& path, const rt::UTexture& texture) {
    const rt::FTexturePlatformData* data = texture.GetPlatformData();
    if (data != nullptr && data->bIsVirtual)
        return readVirtualPixels(path, data->VTData);
    const int first = texture.GetFirstMipIndex();
    if (data == nullptr || first == rt::INDEX_NONE) {
        fail(path, "has no mip with pixels");
        return std::nullopt;
    }
    return readMipPixels(path, *data, first);
}

void checkTexturePixels(const Cook& cook, const std::string& path) {
    rt::UTexture* texture = loadObject<rt::UTexture>(path);
    if (texture == nullptr)
        return;
    const std::string_view name = std::string_view(path).substr(path.rfind('.') + 1);
    const std::optional<Source> source = findSource(name, *texture, cook.game);
    if (!source) {
        fail(path, "has no known source pixels");
        return;
    }
    const std::optional<TexturePixels> pixels = readPixels(path, *texture);
    if (!pixels)
        return;
    const Shape shape{pixels->sizeX, pixels->sizeY, source->seed};
    const Comparison comparison = findComparison(cook, *texture, pixels->format);
    const PixelError error = measureError(pixels->image, *source, shape, pixels->scale, comparison);
    if (error.mean > comparison.tolerance.mean || error.max > comparison.tolerance.max)
        fail(path, std::format("mip {} differs from its source pixels by {:.4f} on average and {:.4f} at most", pixels->mip, error.mean, error.max));
}

bool isFinite(const conversion::DecodedImage& image) {
    return std::all_of(image.rgba.begin(), image.rgba.end(), [](float channel) { return std::isfinite(channel); });
}

//! Lightmass builds the mips of light and shadow maps with coverage weights over padded charts, so only the decoding is checked.
void checkBuiltDataTexture(const std::string& path, const rt::UTexture2D& texture) {
    const rt::FTexturePlatformData* data = texture.GetPlatformData();
    const int first = texture.GetFirstMipIndex();
    if (data == nullptr || first == rt::INDEX_NONE) {
        fail(path, "has no mip with pixels");
        return;
    }
    for (int index = first; index < data->Mips.Num(); index++) {
        const std::optional<conversion::DecodedImage> image = decodeMip(path, *data, index);
        if (!image)
            return;
        if (!isFinite(*image)) {
            fail(path, std::format("mip {} decodes to values that are not finite", index));
            return;
        }
    }
}

void checkBuiltDataTextures(const std::string& path) {
    const std::string packageName = path.substr(0, path.rfind('.'));
    rt::UPackage* package = rt::LoadPackage(rt::FString(packageName));
    if (!CHECK(package != nullptr))
        return;
    rt::TArray<rt::UObject*> objects;
    rt::GetObjectsWithOuter(package, objects, true);
    for (rt::UObject* object : objects)
        if (const rt::UTexture2D* texture = rt::Cast<rt::UTexture2D>(object))
            checkBuiltDataTexture(getPathName(texture), *texture);
}

void checkPixels(const Cook& cook) {
    const Json* assets = cook.expected.find("assets");
    if (assets == nullptr) {
        fail(cook.version, "the reference values have no assets");
        return;
    }
    withRuntime(cook, [&](const ue5parse::FileProvider&) {
        for (const auto& [path, assetClass] : assets->getObject()) {
            const std::string& name = assetClass.getString();
            if (name == "Texture2D" || name == "TextureCube" || name == "Texture2DArray" || name == "VolumeTexture" || name == "TextureCubeArray")
                checkTexturePixels(cook, path);
            if (name == "MapBuildDataRegistry")
                checkBuiltDataTextures(path);
        }
    });
}

}

void addTextureChecks(Checks& checks) {
    checks.cook.emplace("texture.pixels", checkPixels);
    checks.values.emplace("texture.settings", checkSettings);
    checks.values.emplace("texture.size", checkSize);
}

}
