#include "ue5parse/runtime/Engine/Engine/Texture.h"

#include "ue5parse/runtime/Engine/Engine/LightMapTexture2D.h"
#include "ue5parse/runtime/Engine/Engine/Texture2D.h"
#include "ue5parse/runtime/Engine/Engine/TextureCube.h"
#include "ue5parse/runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

void FTexture2DMipMap::Serialize(FArchive& Ar, bool bSerializeMipData) {
    if (bSerializeMipData) BulkData.Serialize(Ar);
    Ar << SizeX << SizeY << SizeZ;
}

void FTexturePlatformData::SerializeCooked(FArchive& Ar, bool bSerializeMipData) {
    // The derived data placeholder: a flag byte (set only for data the cook left in the derived data cache) and 15
    // bytes.
    uint8 bUsingDerivedData = 0;
    Ar << bUsingDerivedData;
    if (bUsingDerivedData != 0) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FTexturePlatformData.DerivedData");
        return;
    }
    Ar.SkipBytes(15);
    Ar << SizeX << SizeY << PackedData;
    FString PixelFormatString;
    Ar << PixelFormatString;
    PixelFormat = GetPixelFormatFromString(PixelFormatString.ToUtf8());
    if (GetHasOptData()) Ar << ExtData << NumMipsInTail;
    int32 FirstMipToSerialize = 0;
    Ar << FirstMipToSerialize;
    int32 NumMips = Ar.ReadCount(12);
    Mips.Empty(NumMips);
    for (int32 MipIndex = 0; MipIndex < NumMips && !Ar.IsError(); MipIndex++) Mips[Mips.AddDefaulted()].Serialize(Ar, bSerializeMipData);
    Ar << bIsVirtual;
    if (Mips.Num() > 0) {
        SizeX = Mips[0].SizeX;
        SizeY = Mips[0].SizeY;
    }
}

void UTexture::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsEditorDataStripped()) Ar.SetError(ErrorCode::UnsupportedVersion, "UTexture.EditorData");
}

void UTexture::SerializeCookedPlatformData(FArchive& Ar, bool bSerializeMipData) {
    if (IsTemplate()) return;
    bool bFound = false;
    FName PixelFormatName;
    Ar << PixelFormatName;
    while (!Ar.IsError() && !PixelFormatName.isNone()) {
        // The offset of the next format, from the position of this field.
        const int64 SkipOffsetPosition = Ar.Tell();
        int64 SkipOffset = 0;
        Ar << SkipOffset;
        const int64 Next = SkipOffsetPosition + SkipOffset;
        if (SkipOffset < 0 || Next > Ar.TotalSize()) {
            Ar.SetError(ErrorCode::InvalidData, "UTexture.PlatformData.Skip");
            return;
        }
        if (!bFound) {
            PlatformData.SerializeCooked(Ar, bSerializeMipData);
            if (Ar.IsError()) return;
            bFound = true;
        }
        Ar.Seek(Next);
        Ar << PixelFormatName;
    }
}

int32 UTexture::GetFirstMipIndex() const {
    for (int32 i = 0; i < PlatformData.Mips.Num(); i++)
        if (PlatformData.Mips[i].BulkData.IsAvailable()) return i;
    return INDEX_NONE;
}

void UTexture2D::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    if (!bCooked) return;
    bool bSerializeMipData = true;
    Ar << bSerializeMipData;
    SerializeCookedPlatformData(Ar, bSerializeMipData);
}

void UTextureCube::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked) SerializeCookedPlatformData(Ar, true);
}

void ULightMapTexture2D::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetRemaining() < 4) return;
    Ar << LightmapFlags;
}

}
