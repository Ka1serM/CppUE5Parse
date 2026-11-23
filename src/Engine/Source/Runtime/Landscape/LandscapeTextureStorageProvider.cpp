#include "Engine/Source/Runtime/Landscape/LandscapeTextureStorageProvider.h"

#include <bit>

#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"
#include "Engine/Source/Runtime/Landscape/LandscapeDataAccess.h"

//! UE builds the normal math without fused multiply-add, so contraction would change the 8-bit normals.
#pragma STDC FP_CONTRACT OFF

namespace ue5parse::runtime {

namespace {

bool IsDuplicateCoord(int32 XY, int32 MipBoundaryOffset) {
    return (XY != 0) && ((XY & (MipBoundaryOffset - 1)) == 0);
}

FVector ComputeTriangleNormal(const FVector& InPoint0, const FVector& InPoint1, const FVector& InPoint2) {
    FVector Normal = (InPoint0 - InPoint1) ^ (InPoint1 - InPoint2);
    Normal.Normalize();
    return Normal;
}

void SampleWorldPositionAtOffset(FVector& OutPoint, const uint8* MipData, int32 X, int32 Y, int32 MipSizeX, const FVector& InLandscapeGridScale) {
    const int64 OffsetBytes = (static_cast<int64>(Y) * MipSizeX + X) * 4;
    const uint16 HeightData = MipData[OffsetBytes + 2] * 256 + MipData[OffsetBytes + 1];
    OutPoint.Set(X * InLandscapeGridScale.X, Y * InLandscapeGridScale.Y, LandscapeDataAccess::GetLocalHeight(HeightData) * InLandscapeGridScale.Z);
}

FVector2f CalculatePremultU16(int32 MipIndex, const FVector& LandscapeGridScale) {
    const int32 MipScale = 1 << MipIndex;
    const float ScaleFactor = -LANDSCAPE_ZSCALE / (LandscapeGridScale.X * LandscapeGridScale.Y * MipScale);
    FVector2f PremultU16;
    PremultU16.X = LandscapeGridScale.Z * LandscapeGridScale.Y * ScaleFactor;
    PremultU16.Y = LandscapeGridScale.Z * LandscapeGridScale.X * ScaleFactor;
    return PremultU16;
}

FVector3f ComputeGridNormalFromDeltaHeightsPremultU16(int32 DHDX, int32 DHDY, const FVector2f& PremultU16) {
    FVector3f Normal;
    Normal.X = DHDX * PremultU16.X;
    Normal.Y = DHDY * PremultU16.Y;
    const float SquareSum = Normal.X * Normal.X + Normal.Y * Normal.Y + 1.0f;
    if (SquareSum > UE_SMALL_NUMBER) {
        const float Scale = FMath::InvSqrtEst(SquareSum);
        Normal.X *= Scale;
        Normal.Y *= Scale;
        Normal.Z = Scale;
        return Normal;
    }
    Normal.X = 0.0f;
    Normal.Y = 0.0f;
    Normal.Z = 1.0f;
    return Normal;
}

uint16 DecodeHeightU16(const FColor* Pixel) {
    return Pixel->R * 256 + Pixel->G;
}

void FastNormalize(FVector3f& V) {
    const float SquareSum = V.X * V.X + V.Y * V.Y + V.Z * V.Z;
    if (SquareSum > UE_SMALL_NUMBER) {
        V *= FMath::InvSqrtEst(SquareSum);
        return;
    }
    V.X = 0.0f;
    V.Y = 0.0f;
    V.Z = 1.0f;
}

void DecodeEdgeNormals(const uint8* Src, uint8* DestData, int32 Width, int32 Height) {
    uint8 LastNormalX = 128;
    uint8 LastNormalY = 128;
    const auto DecodeNormal = [&](int32 X, int32 Y) {
        const int64 DestOffset = (static_cast<int64>(Y) * Width + X) * 4;
        LastNormalX += Src[0];
        LastNormalY += Src[1];
        DestData[DestOffset + 0] = LastNormalX;
        DestData[DestOffset + 3] = LastNormalY;
        Src += 2;
    };
    for (int32 X = 0; X < Width; X++)
        DecodeNormal(X, 0);
    for (int32 Y = 1; Y < Height; Y++)
        DecodeNormal(Width - 1, Y);
    for (int32 X = Width - 2; X >= 0; X--)
        DecodeNormal(X, Height - 1);
    for (int32 Y = Height - 2; Y >= 1; Y--)
        DecodeNormal(0, Y);
}

}

void FLandscapeTexture2DMipMap::Serialize(FArchive& Ar) {
    Ar << SizeX << SizeY << bCompressed;
    BulkData.Serialize(Ar);
}

void ULandscapeTextureStorageProviderFactory::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar << NumNonOptionalMips << NumNonStreamingMips << LandscapeGridScale;
    const int32 NumMips = Ar.ReadCount(1);
    Mips.SetNum(NumMips);
    for (int32 Index = 0; Index < NumMips && !Ar.IsError(); Index++)
        Mips[Index].Serialize(Ar);
    Ar << Texture;
}

Result<std::vector<std::byte>> ULandscapeTextureStorageProviderFactory::GetMipData(int32 MipIndex) const {
    if (!Mips.IsValidIndex(MipIndex))
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.MipIndex");
    if (GetLinker() == nullptr)
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.Linker");
    const FLandscapeTexture2DMipMap& Mip = Mips[MipIndex];
    Result<std::vector<std::byte>> SourceData = Mip.BulkData.Read();
    if (!SourceData)
        return SourceData;
    if (SourceData->empty())
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.BulkDataSize");
    if (!Mip.bCompressed) {
        if (static_cast<int64>(SourceData->size()) != static_cast<int64>(Mip.SizeX) * Mip.SizeY * 4)
            return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.UncompressedSize");
        return SourceData;
    }
    std::vector<std::byte> DestData(static_cast<std::size_t>(Mip.SizeX) * static_cast<std::size_t>(Mip.SizeY) * 4);
    Result<void> Decompressed = DecompressMip(*SourceData, DestData, MipIndex, GetLinker()->GetPackage().getProvider().getGame());
    if (!Decompressed)
        return std::unexpected(Decompressed.error());
    return DestData;
}

Result<void> ULandscapeTextureStorageProviderFactory::DecompressMip(std::span<const std::byte> Source, std::span<std::byte> Dest, int32 MipIndex, EGame Game) const {
    if (!Mips.IsValidIndex(MipIndex) || !Mips[MipIndex].bCompressed)
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.DecompressMip.Mip");
    const FLandscapeTexture2DMipMap& Mip = Mips[MipIndex];
    if (Mip.SizeX < 2 || Mip.SizeY < 2 || MipIndex >= 31)
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.DecompressMip.Size");
    if (static_cast<int64>(Dest.size()) != static_cast<int64>(Mip.SizeX) * Mip.SizeY * 4)
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.DecompressMip.DestDataBytes");
    const uint8* SourceData = reinterpret_cast<const uint8*>(Source.data());
    uint8* DestData = reinterpret_cast<uint8*>(Dest.data());
    const int64 SourceDataBytes = static_cast<int64>(Source.size());
    if (Game < EGame::GAME_UE5_6)
        return DecompressMipOriginalUnoptimized(SourceData, SourceDataBytes, DestData, MipIndex);

    const int32 DestWidth = Mip.SizeX;
    const int32 DestHeight = Mip.SizeY;
    const int32 MipBoundaryOffset = BoundaryOffset >> MipIndex;
    const bool bHasDuplicateData = (BoundaryCountX > 0 || BoundaryCountY > 0) && MipBoundaryOffset > 1;
    const int32 MipBoundaryCountX = bHasDuplicateData ? BoundaryCountX : 0;
    const int32 MipBoundaryCountY = bHasDuplicateData ? BoundaryCountY : 0;
    if (bHasDuplicateData && !std::has_single_bit(static_cast<uint32>(MipBoundaryOffset)))
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.DecompressMip.BoundaryOffset");
    const int32 SrcWidth = DestWidth - MipBoundaryCountX;
    const int32 SrcHeight = DestHeight - MipBoundaryCountY;
    if (SrcWidth < 0 || SrcHeight < 0)
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.DecompressMip.BoundaryCount");
    const int64 NumSrcPixels = static_cast<int64>(SrcWidth) * SrcHeight;
    if (SourceDataBytes != (NumSrcPixels + (static_cast<int64>(DestWidth) + DestHeight) * 2 - 4) * 2)
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.DecompressMip.SourceDataBytes");

    const FVector2f PremultU16 = CalculatePremultU16(MipIndex, LandscapeGridScale);
    uint16 CC = 32768;
    std::vector<FVector3f> PrevLinePartialNormals(static_cast<std::size_t>(DestWidth));
    int32 DuplicateRowsSkipped = 0;
    int32 PreviousRowOffset = 1;
    for (int32 Y = 0; Y < DestHeight; Y++) {
        const uint8* Src = &SourceData[static_cast<int64>(Y - DuplicateRowsSkipped) * SrcWidth * 2];
        FColor* Dst = reinterpret_cast<FColor*>(&DestData[static_cast<int64>(Y) * DestWidth * 4]);
        if (Y == 0) {
            for (int32 X = 0; X < DestWidth; X++) {
                if (bHasDuplicateData && IsDuplicateCoord(X, MipBoundaryOffset)) {
                    Dst++;
                    continue;
                }
                const uint16 DeltaHeight = Src[0] * 256 + Src[1];
                CC += DeltaHeight;
                *Dst = FColor(CC >> 8, CC & 0xff, 128, 128);
                Src += 2;
                Dst++;
            }
            continue;
        }
        if (bHasDuplicateData && IsDuplicateCoord(Y, MipBoundaryOffset)) {
            DuplicateRowsSkipped++;
            PreviousRowOffset = 2;
            continue;
        }
        const int64 TTOffset = static_cast<int64>(DestWidth) * PreviousRowOffset;
        FVector3f P1;
        FVector3f P01;
        uint16 DeltaHeight = Src[0] * 256 + Src[1];
        CC += DeltaHeight;
        *Dst = FColor(CC >> 8, CC & 0xff, 128, 128);
        uint16 TT = DecodeHeightU16(Dst - TTOffset);
        Src += 2;
        Dst++;
        int32 PrevColumnOffset = -1;
        for (int32 X = 1; X < DestWidth; X++) {
            if (bHasDuplicateData && IsDuplicateCoord(X, MipBoundaryOffset)) {
                Dst++;
                PrevColumnOffset = -2;
                continue;
            }
            const uint16 TL = TT;
            const uint16 LL = CC;
            DeltaHeight = Src[0] * 256 + Src[1];
            CC += DeltaHeight;
            TT = DecodeHeightU16(Dst - TTOffset);
            *Dst = FColor(CC >> 8, CC & 0xff, 128, 128);
            const FVector3f N0 = ComputeGridNormalFromDeltaHeightsPremultU16(CC - LL, LL - TL, PremultU16);
            const FVector3f N1 = ComputeGridNormalFromDeltaHeightsPremultU16(TT - TL, CC - TT, PremultU16);
            const FVector3f N01 = N0 + N1;
            FVector3f TL_Normal = PrevLinePartialNormals[static_cast<std::size_t>(X + PrevColumnOffset)] + P1 + N01;
            FastNormalize(TL_Normal);
            Dst[-TTOffset + PrevColumnOffset].B = static_cast<uint8>(FMath::Clamp((TL_Normal.X * 127.5f + 127.5f), 0.0f, 255.0f));
            Dst[-TTOffset + PrevColumnOffset].A = static_cast<uint8>(FMath::Clamp((TL_Normal.Y * 127.5f + 127.5f), 0.0f, 255.0f));
            PrevLinePartialNormals[static_cast<std::size_t>(X + PrevColumnOffset)] = P01 + N0;
            P1 = N1;
            P01 = N01;
            Src += 2;
            Dst++;
            PrevColumnOffset = -1;
        }
        PreviousRowOffset = 1;
    }

    DecodeEdgeNormals(&SourceData[NumSrcPixels * 2], DestData, DestWidth, DestHeight);
    if (!bHasDuplicateData)
        return {};
    FColor* Pixels = reinterpret_cast<FColor*>(DestData);
    for (int32 Y = MipBoundaryOffset; Y < DestHeight; Y += MipBoundaryOffset) {
        FColor* Dst = &Pixels[static_cast<int64>(Y) * DestWidth];
        for (int32 X = 0; X < DestWidth; X++, Dst++)
            *Dst = Dst[-DestWidth];
    }
    for (int32 X = MipBoundaryOffset; X < DestWidth; X += MipBoundaryOffset) {
        FColor* Dst = &Pixels[X];
        for (int32 Y = 0; Y < DestHeight; Y++, Dst += DestWidth)
            *Dst = Dst[-1];
    }
    return {};
}

Result<void> ULandscapeTextureStorageProviderFactory::DecompressMipOriginalUnoptimized(const uint8* SourceData, int64 SourceDataBytes, uint8* DestData,
                                                                                       int32 MipIndex) const {
    const FLandscapeTexture2DMipMap& Mip = Mips[MipIndex];
    const int64 TotalPixels = static_cast<int64>(Mip.SizeX) * Mip.SizeY;
    if (SourceDataBytes != (TotalPixels + (static_cast<int64>(Mip.SizeX) + Mip.SizeY) * 2 - 4) * 2)
        return fail(ErrorCode::InvalidData, "ULandscapeTextureStorageProviderFactory.DecompressMip.SourceDataBytes");
    uint16 LastHeight = 32768;
    for (int64 PixelIndex = 0; PixelIndex < TotalPixels; PixelIndex++) {
        const int64 SourceOffset = PixelIndex * 2;
        const uint16 DeltaHeight = SourceData[SourceOffset + 0] * 256 + SourceData[SourceOffset + 1];
        LastHeight += DeltaHeight;
        const int64 DestOffset = PixelIndex * 4;
        DestData[DestOffset + 0] = 128;
        DestData[DestOffset + 1] = LastHeight & 0xff;
        DestData[DestOffset + 2] = LastHeight >> 8;
        DestData[DestOffset + 3] = 128;
    }
    for (int32 Y = 1; Y < Mip.SizeY - 1; Y++) {
        for (int32 X = 1; X < Mip.SizeX - 1; X++) {
            FVector TL, TT, CC, LL, RR, BR, BB;
            SampleWorldPositionAtOffset(TL, DestData, X - 1, Y - 1, Mip.SizeX, LandscapeGridScale);
            SampleWorldPositionAtOffset(TT, DestData, X + 0, Y - 1, Mip.SizeX, LandscapeGridScale);
            SampleWorldPositionAtOffset(CC, DestData, X + 0, Y + 0, Mip.SizeX, LandscapeGridScale);
            SampleWorldPositionAtOffset(LL, DestData, X - 1, Y + 0, Mip.SizeX, LandscapeGridScale);
            SampleWorldPositionAtOffset(RR, DestData, X + 1, Y + 0, Mip.SizeX, LandscapeGridScale);
            SampleWorldPositionAtOffset(BR, DestData, X + 1, Y + 1, Mip.SizeX, LandscapeGridScale);
            SampleWorldPositionAtOffset(BB, DestData, X + 0, Y + 1, Mip.SizeX, LandscapeGridScale);
            const FVector N0 = ComputeTriangleNormal(CC, LL, TL);
            const FVector N1 = ComputeTriangleNormal(TL, TT, CC);
            const FVector N2 = ComputeTriangleNormal(LL, CC, BB);
            const FVector N3 = ComputeTriangleNormal(RR, CC, TT);
            const FVector N4 = ComputeTriangleNormal(BR, BB, CC);
            const FVector N5 = ComputeTriangleNormal(CC, RR, BR);
            FVector FinalNormal = (N0 + N1 + N2 + N3 + N4 + N5);
            FinalNormal.Normalize();
            const int64 OffsetBytes = (static_cast<int64>(Y) * Mip.SizeX + X) * 4;
            DestData[OffsetBytes + 0] = static_cast<uint8>(FMath::Clamp(((FinalNormal.X + 1.0) * 0.5) * 255.0, 0.0, 255.0));
            DestData[OffsetBytes + 3] = static_cast<uint8>(FMath::Clamp(((FinalNormal.Y + 1.0) * 0.5) * 255.0, 0.0, 255.0));
        }
    }
    DecodeEdgeNormals(&SourceData[TotalPixels * 2], DestData, Mip.SizeX, Mip.SizeY);
    return {};
}

}
