#pragma once

#include "Engine/Source/Runtime/Core/PixelFormat.h"
#include "Engine/Source/Runtime/Engine/Engine/StreamableRenderAsset.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureDefines.h"
#include "Engine/Source/Runtime/Engine/TextureResource.h"

#include "Texture.generated.h"

namespace ue5parse::runtime {

struct FTexturePlatformData {
    int32 SizeX = 0;
    int32 SizeY = 0;
    uint32 PackedData = 0;
    EPixelFormat PixelFormat = EPixelFormat::PF_Unknown;
    uint32 ExtData = 0;
    uint32 NumMipsInTail = 0;
    TArray<FTexture2DMipMap> Mips;
    bool bIsVirtual = false;

    bool GetHasOptData() const { return (PackedData & (1u << 30)) != 0; }
    int32 GetNumSlices() const { return static_cast<int32>(PackedData & ((1u << 29) - 1)); }
    void SerializeCooked(FArchive& Ar, bool bSerializeMipData);
};

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
    const FTexturePlatformData* GetPlatformData() const { return PlatformData.Mips.Num() || PlatformData.SizeX ? &PlatformData : nullptr; }
    int32 GetFirstMipIndex() const;

protected:
    void SerializeCookedPlatformData(FArchive& Ar, bool bSerializeMipData);

    FTexturePlatformData PlatformData;
};

}
