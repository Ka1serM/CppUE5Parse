#include "Engine/Source/Runtime/Landscape/LandscapeTextureStorageProvider.h"

#include <algorithm>
#include <bit>

#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"

namespace ue5parse::runtime {

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

namespace {

template <typename T>
std::byte PackNormal(T Value) {
    return static_cast<std::byte>(std::clamp(Value * T(127.5) + T(127.5), T(0), T(255)));
}

template <typename T>
void WriteInteriorNormal(std::byte* Pixel, const uint16* Center, int32 Width, const TVector<T>& Factors) {
    const auto Face = [&](int32 Dx, int32 Dy) {
        return TVector<T>(Dx * Factors.X, Dy * Factors.Y, Factors.Z).GetSafeNormal();
    };
    const int32 C = Center[0], L = Center[-1], R = Center[1];
    const int32 TLeft = Center[-Width - 1], Top = Center[-Width];
    const int32 Bottom = Center[Width], BRight = Center[Width + 1];
    const auto Normal = (Face(C - L, L - TLeft) + Face(Top - TLeft, C - Top) + Face(C - L, Bottom - C) +
                         Face(R - C, C - Top) + Face(BRight - Bottom, Bottom - C) + Face(R - C, BRight - R)).GetSafeNormal();
    Pixel[0] = PackNormal(Normal.X);
    Pixel[3] = PackNormal(Normal.Y);
}

}

Result<std::vector<std::byte>> ULandscapeTextureStorageProviderFactory::ReadMip(int32 MipIndex) const {
    if (!Mips.IsValidIndex(MipIndex))
        return fail(ErrorCode::InvalidData, "landscape mip index");
    const auto& Mip = Mips[MipIndex];
    const int32 Width = Mip.SizeX, Height = Mip.SizeY;
    if (Width <= 0 || Height <= 0)
        return fail(ErrorCode::InvalidData, "landscape mip dimensions");
    auto Source = Mip.BulkData.Read();
    if (!Source)
        return std::unexpected(Source.error());
    const auto PixelCount = static_cast<std::size_t>(Width) * Height;
    if (!Mip.bCompressed) {
        if (Source->size() != PixelCount * 4)
            return fail(ErrorCode::InvalidData, "uncompressed landscape mip size");
        return Source;
    }
    if (!GetLinker() || Width < 2 || Height < 2 || MipIndex >= 31)
        return fail(ErrorCode::InvalidData, "compressed landscape mip dimensions");
    const EGame Game = GetLinker()->GetPackage().getProvider().getGame();
    int32 Interval = 0;
    if (Game >= EGame::GAME_UE5_8 && (BoundaryCountX || BoundaryCountY)) {
        if (BoundaryOffset < 0 || BoundaryCountX < 0 || BoundaryCountY < 0)
            return fail(ErrorCode::InvalidData, "landscape mip boundaries");
        const int32 Offset = BoundaryOffset >> MipIndex;
        if (Offset > 1) {
            if (!std::has_single_bit(static_cast<uint32>(Offset)) || (Width - 1) / Offset != BoundaryCountX ||
                (Height - 1) / Offset != BoundaryCountY)
                return fail(ErrorCode::InvalidData, "landscape mip boundary count");
            Interval = Offset;
        }
    }
    const auto Compact = [Interval](int32 Coord) { return Interval ? Coord - Coord / Interval : Coord; };
    const int32 Columns = Compact(Width - 1) + 1, Rows = Compact(Height - 1) + 1;
    const auto HeightCount = static_cast<std::size_t>(Rows) * Columns;
    const auto EdgeCount = (static_cast<std::size_t>(Width) + Height - 2) * 2;
    if (Source->size() != 2 * (HeightCount + EdgeCount))
        return fail(ErrorCode::InvalidData, "compressed landscape mip size");

    std::vector<uint16> Heights(HeightCount);
    const std::byte* Next = Source->data();
    uint16 HeightValue = 32768;
    for (auto& Sample : Heights) {
        HeightValue += (std::to_integer<uint16>(Next[0]) << 8) + std::to_integer<uint16>(Next[1]);
        Sample = HeightValue;
        Next += 2;
    }
    const auto& Scale = LandscapeGridScale;
    const FVector LegacyFactors{-Scale.Z * Scale.Y / 128, -Scale.Z * Scale.X / 128, Scale.X * Scale.Y};
    FVector3f Factors;
    if (Game >= EGame::GAME_UE5_6) {
        if (Scale.X == 0 || Scale.Y == 0)
            return fail(ErrorCode::InvalidData, "landscape grid scale");
        const float Factor = static_cast<float>(-1.0 / (128 * Scale.X * Scale.Y * (1u << MipIndex)));
        Factors = {static_cast<float>(Scale.Z * Scale.Y * Factor), static_cast<float>(Scale.Z * Scale.X * Factor), 1};
    }
    if (Scale.ContainsNaN() || LegacyFactors.ContainsNaN() || Factors.ContainsNaN())
        return fail(ErrorCode::InvalidData, "nonfinite landscape grid scale");
    std::vector<std::byte> Output(PixelCount * 4);
    const auto Pixel = [&](int32 X, int32 Y) { return Output.data() + 4 * (static_cast<std::size_t>(Y) * Width + X); };
    for (int32 Y = 0; Y < Height; ++Y) {
        const int32 Row = Compact(Y);
        for (int32 X = 0; X < Width; ++X) {
            const int32 Column = Compact(X);
            const auto* Sample = &Heights[static_cast<std::size_t>(Row) * Columns + Column];
            auto* Dst = Pixel(X, Y);
            Dst[1] = static_cast<std::byte>(*Sample & 255);
            Dst[2] = static_cast<std::byte>(*Sample >> 8);
            if (Row > 0 && Row < Rows - 1 && Column > 0 && Column < Columns - 1) {
                if (Game >= EGame::GAME_UE5_6)
                    WriteInteriorNormal(Dst, Sample, Columns, Factors);
                else
                    WriteInteriorNormal(Dst, Sample, Columns, LegacyFactors);
            }
        }
    }
    uint8 NormalX = 128, NormalY = 128;
    const auto Edge = [&](int32 X, int32 Y) {
        NormalX += std::to_integer<uint8>(*Next++);
        NormalY += std::to_integer<uint8>(*Next++);
        Pixel(X, Y)[0] = static_cast<std::byte>(NormalX);
        Pixel(X, Y)[3] = static_cast<std::byte>(NormalY);
    };
    for (int32 X = 0; X < Width; ++X)
        Edge(X, 0);
    for (int32 Y = 1; Y < Height; ++Y)
        Edge(Width - 1, Y);
    for (int32 X = Width - 2; X >= 0; --X)
        Edge(X, Height - 1);
    for (int32 Y = Height - 2; Y >= 1; --Y)
        Edge(0, Y);
    if (Interval) {
        for (int64 Y = Interval; Y < Height; Y += Interval)
            std::copy_n(Pixel(0, static_cast<int32>(Y) - 1), static_cast<std::size_t>(Width) * 4, Pixel(0, static_cast<int32>(Y)));
        for (int64 X = Interval; X < Width; X += Interval)
            for (int32 Y = 0; Y < Height; ++Y)
                std::copy_n(Pixel(static_cast<int32>(X) - 1, Y), 4, Pixel(static_cast<int32>(X), Y));
    }
    return Output;
}

}
