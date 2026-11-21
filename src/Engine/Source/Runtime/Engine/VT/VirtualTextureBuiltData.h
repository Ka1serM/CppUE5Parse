#pragma once

#include <optional>

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Core/Misc/SecureHash.h"
#include "Engine/Source/Runtime/Core/PixelFormat.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"

namespace ue5parse::runtime {

inline constexpr uint32 VIRTUALTEXTURE_DATA_MAXLAYERS = 8u;

enum class EVirtualTextureCodec : uint8 {
    Black,
    OpaqueBlack,
    White,
    Flat,
    RawGPU,
    ZippedGPU,
    Crunch,
    Max,
};

struct FVirtualTextureDataChunk {
    FByteBulkData BulkData;
    FSHAHash BulkDataHash;
    uint32 SizeInBytes = 0;
    uint32 CodecPayloadSize = 0;
    uint32 CodecPayloadOffset[VIRTUALTEXTURE_DATA_MAXLAYERS] = {};
    EVirtualTextureCodec CodecType[VIRTUALTEXTURE_DATA_MAXLAYERS] = {};
};

struct FVirtualTextureTileOffsetData {
    uint32 Width = 0;
    uint32 Height = 0;
    uint32 MaxAddress = 0;
    TArray<uint32> Addresses;
    TArray<uint32> Offsets;

    uint32 GetTileOffset(uint32 InAddress) const;
};

FArchive& operator<<(FArchive& Ar, FVirtualTextureTileOffsetData& TileOffsetData);

struct FVirtualTextureTileLayer {
    int32 ChunkIndex = -1;
    uint32 Offset = 0;
    uint32 Size = 0;
    EVirtualTextureCodec Codec = EVirtualTextureCodec::Black;
};

struct FVirtualTextureBuiltData {
    uint32 NumLayers = 0;
    uint32 NumMips = 0;
    uint32 Width = 0;
    uint32 Height = 0;
    uint32 WidthInBlocks = 0;
    uint32 HeightInBlocks = 0;
    uint32 TileSize = 0;
    uint32 TileBorderSize = 0;
    EPixelFormat LayerTypes[VIRTUALTEXTURE_DATA_MAXLAYERS] = {};
    FLinearColor LayerFallbackColors[VIRTUALTEXTURE_DATA_MAXLAYERS];
    TArray<FVirtualTextureDataChunk> Chunks;
    TArray<uint32> TileDataOffsetPerLayer;
    TArray<uint32> ChunkIndexPerMip;
    TArray<uint32> BaseOffsetPerMip;
    TArray<FVirtualTextureTileOffsetData> TileOffsetData;
    TArray<uint32> TileIndexPerChunk;
    TArray<uint32> TileIndexPerMip;
    TArray<uint32> TileOffsetInChunk;

    uint32 GetPhysicalTileSize() const { return TileSize + TileBorderSize * 2u; }
    uint32 GetWidthInTiles(uint32 vLevel) const;
    uint32 GetHeightInTiles(uint32 vLevel) const;
    void Serialize(FArchive& Ar);
    std::optional<FVirtualTextureTileLayer> FindTileLayer(uint32 vLevel, uint32 vAddress, uint32 LayerIndex) const;

private:
    bool IsLegacyData() const { return TileOffsetInChunk.Num() > 0; }
    uint32 GetTileIndex_Legacy(uint32 vLevel, uint32 vAddress) const;
    int32 GetChunkIndex_Legacy(uint32 TileIndex) const;
    uint32 GetTileOffset_Legacy(int32 ChunkIndex, uint32 TileIndex) const;
    std::optional<FVirtualTextureTileLayer> FindTileLayer_Legacy(uint32 vLevel, uint32 vAddress, uint32 LayerIndex) const;
};

}
