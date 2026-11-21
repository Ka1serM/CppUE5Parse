#include "Engine/Source/Runtime/Engine/Engine/Texture.h"

#include "Engine/Source/Runtime/Engine/Engine/LightMapTexture2D.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureCube.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

namespace {

bool IsUE4(const FArchive& Ar) {
    return Ar.GetGame() < EGame::GAME_UE5_0;
}

}

void FTexture2DMipMap::Serialize(FArchive& Ar, bool bSerializeMipData) {
    if (IsUE4(Ar)) {
        bool bCooked = false;
        Ar << bCooked;
    }
    if (bSerializeMipData)
        BulkData.Serialize(Ar);
    Ar << SizeX << SizeY;
    if (Ar.GetGame() >= EGame::GAME_UE4_20)
        Ar << SizeZ;
}

void FTextureCpuImage::Serialize(FArchive& Ar) {
    int64 NumBytes = 0;
    Ar << SizeX << SizeY << NumSlices << Format << GammaSpace << NumBytes;
    if (Ar.IsError())
        return;
    if (NumBytes < 0 || NumBytes > Ar.GetRemaining() || static_cast<uint64>(NumBytes) > RawData.max_size()) {
        Ar.SetError(ErrorCode::InvalidData, "FTextureCpuImage.RawData");
        return;
    }
    RawData.resize(static_cast<size_t>(NumBytes));
    if (NumBytes > 0)
        Ar.Serialize(RawData.data(), NumBytes);
}

void FTexturePlatformData::SerializeCooked(FArchive& Ar, bool bSerializeMipData) {
    if (!IsUE4(Ar)) {
        uint8 bUsingDerivedData = 0;
        Ar << bUsingDerivedData;
        if (bUsingDerivedData != 0) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "FTexturePlatformData.DerivedData");
            return;
        }
        Ar.SkipBytes(15);
    }
    Ar << SizeX << SizeY << PackedData;
    FString PixelFormatString;
    Ar << PixelFormatString;
    const int64 PixelFormatValue = UTexture::GetPixelFormatEnum()->GetValueByName(FName(PixelFormatString.ToUtf8()));
    PixelFormat = PixelFormatValue == INDEX_NONE ? EPixelFormat::PF_Unknown : static_cast<EPixelFormat>(PixelFormatValue);
    if (GetHasOptData() && (Ar.GetGame() < EGame::GAME_UE5_6 || bSerializeMipData))
        Ar << ExtData << NumMipsInTail;
    if (Ar.GetGame() >= EGame::GAME_UE5_4 && (PackedData & (1u << 29)) != 0) {
        CPUCopy.Serialize(Ar);
        if (Ar.IsError())
            return;
    }
    int32 FirstMipToSerialize = 0;
    Ar << FirstMipToSerialize;
    int32 NumMips = Ar.ReadCount(12);
    Mips.Empty(NumMips);
    for (int32 MipIndex = 0; MipIndex < NumMips && !Ar.IsError(); MipIndex++)
        Mips[Mips.AddDefaulted()].Serialize(Ar, bSerializeMipData);
    if (Ar.GetGame() >= EGame::GAME_UE4_23)
        Ar << bIsVirtual;
    if (bIsVirtual)
        VTData.Serialize(Ar);
    if (Mips.Num() > 0) {
        SizeX = Mips[0].SizeX;
        SizeY = Mips[0].SizeY;
    }
}

UEnum* UTexture::GetPixelFormatEnum() {
    static constinit std::atomic<UEnum*> Cache = nullptr;
    UEnum* Enum = Cache.load(std::memory_order_acquire);
    return Enum ? Enum : FindNativeEnum(Cache, "EPixelFormat");
}

void UTexture::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsEditorDataStripped())
        Ar.SetError(ErrorCode::UnsupportedVersion, "UTexture.EditorData");
}

void UTexture::SerializeCookedPlatformData(FArchive& Ar, bool bSerializeMipData) {
    if (IsTemplate())
        return;
    bool bFound = false;
    FName PixelFormatName;
    Ar << PixelFormatName;
    while (!Ar.IsError() && !PixelFormatName.isNone()) {
        const int64 SkipOffsetPosition = Ar.Tell();
        int64 SkipOffset = 0;
        if (Ar.GetGame() >= EGame::GAME_UE4_20) {
            Ar << SkipOffset;
        } else {
            int32 SkipOffset32 = 0;
            Ar << SkipOffset32;
            SkipOffset = SkipOffset32;
        }
        //! UE4 stores the absolute position in the package file, UE5 the distance from the offset itself.
        const int64 Next = IsUE4(Ar) ? SkipOffset - (Ar.GetCookedPosition() - Ar.Tell()) : SkipOffsetPosition + SkipOffset;
        if (Next < SkipOffsetPosition || Next > Ar.TotalSize()) {
            Ar.SetError(ErrorCode::InvalidData, "UTexture.PlatformData.Skip");
            return;
        }
        if (!bFound) {
            PlatformData.SerializeCooked(Ar, bSerializeMipData);
            if (Ar.IsError())
                return;
            bFound = true;
        }
        Ar.Seek(Next);
        Ar << PixelFormatName;
    }
}

int32 UTexture::GetFirstMipIndex() const {
    for (int32 i = 0; i < PlatformData.Mips.Num(); i++)
        if (PlatformData.Mips[i].BulkData.IsAvailable())
            return i;
    return INDEX_NONE;
}

void UTexture2D::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    if (!bCooked)
        return;
    bool bSerializeMipData = true;
    if (Ar.GetGame() >= EGame::GAME_UE5_3)
        Ar << bSerializeMipData;
    SerializeCookedPlatformData(Ar, bSerializeMipData);
}

void UTextureCube::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        SerializeCookedPlatformData(Ar, true);
}

void ULightMapTexture2D::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetRemaining() < 4)
        return;
    Ar << LightmapFlags;
}

}
