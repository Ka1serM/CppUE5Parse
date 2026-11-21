#include "Engine/Source/Runtime/Engine/VT/VirtualTextureBuiltData.h"

#include <algorithm>

#include "Engine/Source/Runtime/Engine/Engine/Texture.h"

namespace ue5parse::runtime {

namespace {

//! UE 4.27 widens the codec payload offset of a chunk from uint16 to uint32.
void SerializeCodecPayloadOffset(FArchive& Ar, uint32& Offset) {
    if (Ar.GetGame() >= EGame::GAME_UE4_27) {
        Ar << Offset;
        return;
    }
    uint16 Offset16 = 0;
    Ar << Offset16;
    Offset = Offset16;
}

}

uint32 FVirtualTextureTileOffsetData::GetTileOffset(uint32 InAddress) const {
    const auto Block = std::upper_bound(Addresses.begin(), Addresses.end(), InAddress);
    if (Block == Addresses.begin())
        return ~0u;
    const int32 BlockIndex = static_cast<int32>(Block - Addresses.begin()) - 1;
    const uint32 BaseOffset = Offsets[BlockIndex];
    if (BaseOffset == ~0u)
        return ~0u;
    return BaseOffset + (InAddress - Addresses[BlockIndex]);
}

FArchive& operator<<(FArchive& Ar, FVirtualTextureTileOffsetData& TileOffsetData) {
    return Ar << TileOffsetData.Width << TileOffsetData.Height << TileOffsetData.MaxAddress << TileOffsetData.Addresses
              << TileOffsetData.Offsets;
}

uint32 FVirtualTextureBuiltData::GetWidthInTiles(uint32 vLevel) const {
    const uint32 LevelWidth = std::max(Width >> vLevel, 1u);
    return (LevelWidth + TileSize - 1) / TileSize;
}

uint32 FVirtualTextureBuiltData::GetHeightInTiles(uint32 vLevel) const {
    const uint32 LevelHeight = std::max(Height >> vLevel, 1u);
    return (LevelHeight + TileSize - 1) / TileSize;
}

void FVirtualTextureBuiltData::Serialize(FArchive& Ar) {
    const bool bHasTileOffsetData = Ar.GetGame() >= EGame::GAME_UE5_0;
    bool bCooked = false;
    Ar << bCooked;
    if (!bCooked) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FVirtualTextureBuiltData.EditorData");
        return;
    }
    Ar << NumLayers << WidthInBlocks << HeightInBlocks << TileSize << TileBorderSize;
    if (bHasTileOffsetData)
        Ar << TileDataOffsetPerLayer;
    Ar << NumMips << Width << Height;
    if (bHasTileOffsetData)
        Ar << ChunkIndexPerMip << BaseOffsetPerMip << TileOffsetData;
    Ar << TileIndexPerChunk << TileIndexPerMip << TileOffsetInChunk;
    if (NumLayers > VIRTUALTEXTURE_DATA_MAXLAYERS) {
        Ar.SetError(ErrorCode::InvalidData, "FVirtualTextureBuiltData.NumLayers");
        return;
    }
    for (uint32 Layer = 0; Layer < NumLayers; Layer++) {
        FString PixelFormatString;
        Ar << PixelFormatString;
        const int64 PixelFormatValue = UTexture::GetPixelFormatEnum()->GetValueByName(FName(PixelFormatString.ToUtf8()));
        LayerTypes[Layer] = PixelFormatValue == INDEX_NONE ? EPixelFormat::PF_Unknown : static_cast<EPixelFormat>(PixelFormatValue);
    }
    if (bHasTileOffsetData) {
        for (uint32 Layer = 0; Layer < NumLayers; Layer++)
            Ar << LayerFallbackColors[Layer];
    }
    const int32 NumChunks = Ar.ReadCount(1);
    Chunks.Empty(NumChunks);
    for (int32 ChunkIndex = 0; ChunkIndex < NumChunks && !Ar.IsError(); ChunkIndex++) {
        FVirtualTextureDataChunk& Chunk = Chunks[Chunks.AddDefaulted()];
        if (bHasTileOffsetData)
            Ar << Chunk.BulkDataHash;
        Ar << Chunk.SizeInBytes << Chunk.CodecPayloadSize;
        for (uint32 Layer = 0; Layer < NumLayers; Layer++) {
            uint8 Codec = 0;
            Ar << Codec;
            SerializeCodecPayloadOffset(Ar, Chunk.CodecPayloadOffset[Layer]);
            Chunk.CodecType[Layer] = static_cast<EVirtualTextureCodec>(Codec);
        }
        Chunk.BulkData.Serialize(Ar);
    }
}

std::optional<FVirtualTextureTileLayer> FVirtualTextureBuiltData::FindTileLayer(uint32 vLevel, uint32 vAddress, uint32 LayerIndex) const {
    if (vLevel >= NumMips || LayerIndex >= NumLayers)
        return std::nullopt;
    if (IsLegacyData())
        return FindTileLayer_Legacy(vLevel, vAddress, LayerIndex);
    const int32 Level = static_cast<int32>(vLevel);
    if (!ChunkIndexPerMip.IsValidIndex(Level) || !BaseOffsetPerMip.IsValidIndex(Level) || !TileOffsetData.IsValidIndex(Level))
        return std::nullopt;
    const uint32 BaseOffset = BaseOffsetPerMip[Level];
    const uint32 TileOffset = TileOffsetData[Level].GetTileOffset(vAddress);
    if (BaseOffset == ~0u || TileOffset == ~0u)
        return std::nullopt;
    const int32 ChunkIndex = static_cast<int32>(ChunkIndexPerMip[Level]);
    if (!Chunks.IsValidIndex(ChunkIndex))
        return std::nullopt;
    const uint32 LayerBegin = LayerIndex == 0 ? 0u : TileDataOffsetPerLayer[static_cast<int32>(LayerIndex) - 1];
    const uint32 LayerEnd = TileDataOffsetPerLayer[static_cast<int32>(LayerIndex)];
    return FVirtualTextureTileLayer{ChunkIndex, BaseOffset + TileOffset * TileDataOffsetPerLayer.Last() + LayerBegin, LayerEnd - LayerBegin,
                                    Chunks[ChunkIndex].CodecType[LayerIndex]};
}

uint32 FVirtualTextureBuiltData::GetTileIndex_Legacy(uint32 vLevel, uint32 vAddress) const {
    const uint32 TileIndex = TileIndexPerMip[static_cast<int32>(vLevel)] + vAddress * NumLayers;
    if (TileIndex >= TileIndexPerMip[static_cast<int32>(vLevel) + 1])
        return ~0u;
    return TileIndex;
}

int32 FVirtualTextureBuiltData::GetChunkIndex_Legacy(uint32 TileIndex) const {
    for (int32 ChunkIndex = 0; ChunkIndex + 1 < TileIndexPerChunk.Num(); ChunkIndex++) {
        if (TileIndex < TileIndexPerChunk[ChunkIndex + 1])
            return ChunkIndex;
    }
    return -1;
}

uint32 FVirtualTextureBuiltData::GetTileOffset_Legacy(int32 ChunkIndex, uint32 TileIndex) const {
    if (TileIndex < TileIndexPerChunk[ChunkIndex + 1])
        return TileOffsetInChunk[static_cast<int32>(TileIndex)];
    return Chunks[ChunkIndex].SizeInBytes;
}

std::optional<FVirtualTextureTileLayer> FVirtualTextureBuiltData::FindTileLayer_Legacy(uint32 vLevel, uint32 vAddress, uint32 LayerIndex) const {
    if (static_cast<int32>(vLevel) + 1 >= TileIndexPerMip.Num())
        return std::nullopt;
    const uint32 TileIndex = GetTileIndex_Legacy(vLevel, vAddress);
    if (TileIndex == ~0u || TileIndex >= static_cast<uint32>(TileOffsetInChunk.Num()))
        return std::nullopt;
    const int32 ChunkIndex = GetChunkIndex_Legacy(TileIndex);
    if (!Chunks.IsValidIndex(ChunkIndex))
        return std::nullopt;
    if (GetTileOffset_Legacy(ChunkIndex, TileIndex) == GetTileOffset_Legacy(ChunkIndex, TileIndex + NumLayers))
        return std::nullopt;
    const uint32 LayerBegin = GetTileOffset_Legacy(ChunkIndex, TileIndex + LayerIndex);
    const uint32 LayerEnd = GetTileOffset_Legacy(ChunkIndex, TileIndex + LayerIndex + 1);
    return FVirtualTextureTileLayer{ChunkIndex, LayerBegin, LayerEnd - LayerBegin, Chunks[ChunkIndex].CodecType[LayerIndex]};
}

}
