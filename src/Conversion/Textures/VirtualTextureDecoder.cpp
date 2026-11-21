#include "Conversion/Textures/VirtualTextureDecoder.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <vector>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::conversion {

namespace {

using runtime::EVirtualTextureCodec;
using runtime::FVirtualTextureBuiltData;
using runtime::FVirtualTextureTileLayer;
using Rgba = std::array<float, 4>;

std::uint32_t encodeMorton(std::uint32_t x, std::uint32_t y) {
    std::uint32_t address = 0;
    for (std::uint32_t bit = 0; bit < 16; bit++)
        address |= ((x >> bit) & 1u) << (bit * 2) | ((y >> bit) & 1u) << (bit * 2 + 1);
    return address;
}

//! The uniform codecs stand for these 8-bit colors (VirtualTextureTranscodeCache.cpp).
std::optional<Rgba> findUniformColor(EVirtualTextureCodec codec) {
    switch (codec) {
    case EVirtualTextureCodec::Black:
        return Rgba{0, 0, 0, 0};
    case EVirtualTextureCodec::OpaqueBlack:
        return Rgba{0, 0, 0, 1};
    case EVirtualTextureCodec::White:
        return Rgba{1, 1, 1, 1};
    case EVirtualTextureCodec::Flat:
        return Rgba{127.0f / 255.0f, 127.0f / 255.0f, 1, 1};
    default:
        return std::nullopt;
    }
}

class TileReader {
public:
    TileReader(const FVirtualTextureBuiltData& data, unsigned layer) : data(data), layout{data.LayerTypes[layer], 0, 0, 1}, chunks(data.Chunks.Num()) {
        layout.sizeX = layout.sizeY = static_cast<int>(data.GetPhysicalTileSize());
    }

    ConversionResult<DecodedImage> read(const FVirtualTextureTileLayer& tile) {
        if (const std::optional<Rgba> color = findUniformColor(tile.Codec))
            return fill(*color);
        const ConversionResult<std::span<const std::byte>> chunk = readChunk(tile.ChunkIndex);
        if (!chunk)
            return std::unexpected(chunk.error());
        if (static_cast<std::size_t>(tile.Offset) + tile.Size > chunk->size())
            return conversionFailure(std::format("the tile at {} with {} bytes is past the end of chunk {}", tile.Offset, tile.Size, tile.ChunkIndex));
        const std::span<const std::byte> payload = chunk->subspan(tile.Offset, tile.Size);
        if (tile.Codec == EVirtualTextureCodec::RawGPU)
            return decodePixels(layout, payload);
        if (tile.Codec != EVirtualTextureCodec::ZippedGPU)
            return conversionFailure(std::format("the virtual texture codec {} has no decoder", static_cast<int>(tile.Codec)));
        return unzip(payload);
    }

private:
    ConversionResult<std::span<const std::byte>> readChunk(int32_t index) {
        std::optional<std::vector<std::byte>>& chunk = chunks[static_cast<std::size_t>(index)];
        if (chunk)
            return std::span<const std::byte>(*chunk);
        Result<std::vector<std::byte>> bytes = data.Chunks[index].BulkData.Read();
        if (!bytes)
            return conversionFailure(bytes.error());
        chunk = std::move(*bytes);
        return std::span<const std::byte>(*chunk);
    }

    ConversionResult<DecodedImage> unzip(std::span<const std::byte> payload) {
        const Decompressor* zlib = runtime::GetRuntimeProvider().getFiles().getCodecs().find(FName("Zlib"));
        if (zlib == nullptr)
            return conversionFailure("there is no Zlib decompressor");
        unzipped.resize(layout.getSize());
        if (Result<void> result = zlib->decompress(payload, unzipped); !result)
            return conversionFailure(result.error());
        return decodePixels(layout, unzipped);
    }

    DecodedImage fill(const Rgba& color) const {
        DecodedImage image{layout.sizeX, layout.sizeY, 1, {}};
        image.rgba.resize(static_cast<std::size_t>(layout.sizeX) * layout.sizeY * 4);
        for (std::size_t pixel = 0; pixel < image.rgba.size(); pixel += 4)
            std::copy(color.begin(), color.end(), image.rgba.begin() + static_cast<std::ptrdiff_t>(pixel));
        return image;
    }

    const FVirtualTextureBuiltData& data;
    PixelLayout layout;
    std::vector<std::optional<std::vector<std::byte>>> chunks;
    std::vector<std::byte> unzipped;
};

}

ConversionResult<DecodedImage> decodeVirtualTexture(const FVirtualTextureBuiltData& data, unsigned layer, unsigned level) {
    if (layer >= data.NumLayers || level >= data.NumMips || data.TileSize == 0)
        return conversionFailure(std::format("the virtual texture has no layer {} at mip {}", layer, level));
    const int tileSize = static_cast<int>(data.TileSize);
    const int border = static_cast<int>(data.TileBorderSize);
    DecodedImage image{static_cast<int>(std::max(data.Width >> level, 1u)), static_cast<int>(std::max(data.Height >> level, 1u)), 1, {}};
    image.rgba.resize(static_cast<std::size_t>(image.sizeX) * image.sizeY * 4);
    TileReader reader(data, layer);
    for (std::uint32_t tileY = 0; tileY < data.GetHeightInTiles(level); tileY++)
        for (std::uint32_t tileX = 0; tileX < data.GetWidthInTiles(level); tileX++) {
            const std::optional<FVirtualTextureTileLayer> tile = data.FindTileLayer(level, encodeMorton(tileX, tileY), layer);
            if (!tile)
                continue;
            const ConversionResult<DecodedImage> pixels = reader.read(*tile);
            if (!pixels)
                return std::unexpected(pixels.error());
            const int originX = static_cast<int>(tileX) * tileSize;
            const int originY = static_cast<int>(tileY) * tileSize;
            const int columns = std::min(tileSize, image.sizeX - originX);
            const int rows = std::min(tileSize, image.sizeY - originY);
            for (int row = 0; row < rows; row++) {
                const std::span<const float, 4> first = pixels->getPixel(border, border + row, 0);
                const std::size_t target = (static_cast<std::size_t>(originY + row) * image.sizeX + originX) * 4;
                std::copy_n(first.data(), columns * 4, image.rgba.data() + target);
            }
        }
    return image;
}

}
