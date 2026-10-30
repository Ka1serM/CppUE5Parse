#pragma once

#include "ue5parse/runtime/Core/PixelFormat.h"
#include "ue5parse/runtime/Engine/Engine/StreamableRenderAsset.h"
#include "ue5parse/runtime/Engine/Engine/TextureDefines.h"
#include "ue5parse/runtime/Engine/TextureResource.h"

#include "Texture.generated.h"

namespace ue5parse::runtime {

// FTexturePlatformData: the cooked data of one pixel format: the size, the format, the mips.
struct FTexturePlatformData {
    int32 SizeX = 0;
    int32 SizeY = 0;
    // The number of slices, and the cube map, opt data and CPU copy bits.
    uint32 PackedData = 0;
    EPixelFormat PixelFormat = EPixelFormat::PF_Unknown;
    uint32 ExtData = 0;
    uint32 NumMipsInTail = 0;
    TArray<FTexture2DMipMap> Mips;
    // A virtual texture keeps its pixels in FVirtualTextureBuiltData, which the runtime does not read.
    bool bIsVirtual = false;

    bool IsCubemap() const { return (PackedData & (1u << 31)) != 0; }
    bool GetHasOptData() const { return (PackedData & (1u << 30)) != 0; }
    bool GetHasCpuCopy() const { return (PackedData & (1u << 29)) != 0; }
    int32 GetNumSlices() const { return static_cast<int32>(PackedData & ((1u << 29) - 1)); }
    int32 GetNumMipsInTail() const { return static_cast<int32>(NumMipsInTail); }
    // UE's FTexturePlatformData::SerializeCooked.
    void SerializeCooked(FArchive& Ar, bool bSerializeMipData);
};

// UTexture: the base of textures (UE's Engine/Texture.h).
UCLASS(Abstract)
class UTexture : public UStreamableRenderAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid LightingGuid;
    UPROPERTY()
    int32 LODBias = 0;
    UPROPERTY()
    TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_Default;
    UPROPERTY()
    TEnumAsByte<TextureFilter> Filter = TF_Default;
    UPROPERTY()
    TEnumAsByte<TextureGroup> LODGroup = TEXTUREGROUP_World;
    UPROPERTY()
    bool SRGB = true;
    UPROPERTY()
    bool VirtualTextureStreaming = false;
    UPROPERTY()
    bool CompressionYCoCg = false;

    void Serialize(FArchive& Ar) override;
    // The platform data that was read: the first pixel format the cook stored. Null for a texture without cooked data.
    const FTexturePlatformData* GetPlatformData() const { return PlatformData.Mips.Num() || PlatformData.SizeX ? &PlatformData : nullptr; }
    FTexturePlatformData* GetRunningPlatformData() { return &PlatformData; }
    EPixelFormat GetPixelFormat() const { return PlatformData.PixelFormat; }
    bool IsCurrentlyVirtualTextured() const { return PlatformData.bIsVirtual; }
    // The first mip whose payload can be read; INDEX_NONE when none can. Reads nothing.
    int32 GetFirstMipIndex() const;

protected:
    // Reads the cooked platform data of every pixel format the cook stored; keeps the first (UE's
    // UTexture::SerializeCookedPlatformData).
    void SerializeCookedPlatformData(FArchive& Ar, bool bSerializeMipData);

    FTexturePlatformData PlatformData;
};

}
