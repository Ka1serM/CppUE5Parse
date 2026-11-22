#include <utility>

#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/PackedNormal.h"
#include "Engine/Source/Runtime/Engine/RawIndexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/ColorVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/PositionVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/StaticMeshVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

namespace {

bool ReadBulkHeader(FArchive& Ar, int32 ElementSize, int32& OutCount) {
    int32 SerializedElementSize = 0;
    Ar << SerializedElementSize;
    OutCount = Ar.ReadCount(ElementSize);
    if (Ar.IsError())
        return false;
    if (OutCount > 0 && SerializedElementSize != ElementSize) {
        Ar.SetError(ErrorCode::InvalidData, "BulkSerialize.ElementSize");
        return false;
    }
    return true;
}

}

namespace {

template <typename DecodeType>
void ReadPositions(FArchive& Ar, int32 ElementSize, TArray<FVector3f>& OutPositions, DecodeType&& Decode) {
    int32 Count = 0;
    if (!ReadBulkHeader(Ar, ElementSize, Count))
        return;
    OutPositions.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++)
        OutPositions.Add(Decode(Ar));
}

void ReadFullPositions(FArchive& Ar, TArray<FVector3f>& OutPositions) {
    int32 Count = 0;
    if (!ReadBulkHeader(Ar, sizeof(FVector3f), Count))
        return;
    SerializeRaw(Ar, OutPositions, Count);
}

FVector3f ReadFloatPosition(FArchive& Ar) {
    FVector3f Position;
    Ar << Position.X << Position.Y << Position.Z;
    return Position;
}

//! Four halves; the fourth is padding.
FVector3f ReadHalfPosition(FArchive& Ar) {
    uint16 X = 0, Y = 0, Z = 0, W = 0;
    Ar << X << Y << Z << W;
    return FVector3f(HalfToFloat(X), HalfToFloat(Y), HalfToFloat(Z));
}

//! Four halves; the fourth scales the other three.
FVector3f ReadScaledHalfPosition(FArchive& Ar) {
    uint16 X = 0, Y = 0, Z = 0, W = 0;
    Ar << X << Y << Z << W;
    const float Scale = HalfToFloat(W);
    return FVector3f(HalfToFloat(X) * Scale, HalfToFloat(Y) * Scale, HalfToFloat(Z) * Scale);
}

FVector3f ReadUnsignedShortPosition(FArchive& Ar) {
    uint16 X = 0, Y = 0, Z = 0, W = 0;
    Ar << X << Y << Z << W;
    return FVector3f(X, Y, Z);
}

//! Four signed shorts; the fourth divides the other three.
FVector3f ReadSignedShortPosition(FArchive& Ar) {
    int16 X = 0, Y = 0, Z = 0, W = 0;
    Ar << X << Y << Z << W;
    const float Scale = W == 0 ? 1.0f : static_cast<float>(W);
    return FVector3f(X / Scale, Y / Scale, Z / Scale);
}

//! Three 10-bit values.
FVector3f ReadPacked32Position(FArchive& Ar) {
    uint32 Data = 0;
    Ar << Data;
    return FVector3f(static_cast<float>(Data & 0x3FF), static_cast<float>((Data >> 10) & 0x3FF), static_cast<float>((Data >> 20) & 0x3FF));
}

void SerializeHonorofKingsWorldPositions(FArchive& Ar, uint32& Stride, uint32& NumVertices, TArray<FVector3f>& OutPositions) {
    int32 Size = 0;
    Ar << Size;
    if (Size == 12) {
        Stride = 12;
        Ar << NumVertices;
        ReadFullPositions(Ar, OutPositions);
        return;
    }
    if (Size != 32) {
        Ar.SetError(ErrorCode::InvalidData, "FPositionVertexBuffer.HonorofKingsWorld");
        return;
    }
    Ar << NumVertices;
    const FVector3f Origin = ReadFloatPosition(Ar);
    const FVector3f Extent = ReadFloatPosition(Ar);
    Stride = 8;
    ReadPositions(Ar, 8, OutPositions, [&](FArchive& InAr) {
        const FVector3f Packed = ReadUnsignedShortPosition(InAr);
        return FVector3f(Packed.X * Extent.X / 65536 + Origin.X, Packed.Y * Extent.Y / 65536 + Origin.Y, Packed.Z * Extent.Z / 65536 + Origin.Z);
    });
}

void SerializeSuicideSquadPositions(FArchive& Ar, uint32& Stride, uint32& NumVertices, TArray<FVector3f>& OutPositions) {
    Ar << Stride << NumVertices;
    Ar.SkipBytes(1);
    const FVector3f Scale = ReadFloatPosition(Ar);
    const FVector3f Offset = ReadFloatPosition(Ar);
    if (Stride == 12) {
        ReadFullPositions(Ar, OutPositions);
        return;
    }
    ReadPositions(Ar, 6, OutPositions, [&](FArchive& InAr) {
        uint16 X = 0, Y = 0, Z = 0;
        InAr << X << Y << Z;
        return FVector3f(X / Scale.X - Offset.X, Y / Scale.Y - Offset.Y, Z / Scale.Z - Offset.Z);
    });
}

bool SerializeGamePositions(FArchive& Ar, uint32& Stride, uint32& NumVertices, TArray<FVector3f>& OutPositions) {
    switch (Ar.GetGame()) {
    case EGame::GAME_Undawn:
    case EGame::GAME_RacingMaster: {
        if (Ar.GetGame() == EGame::GAME_Undawn) {
            bool bUseFullPrecisionPositions = false;
            Ar << bUseFullPrecisionPositions;
        }
        Ar << Stride << NumVertices;
        //! Undawn stores half positions even when its flag says full precision.
        if (Ar.GetGame() == EGame::GAME_RacingMaster && Stride == 12)
            ReadFullPositions(Ar, OutPositions);
        else
            ReadPositions(Ar, 8, OutPositions, ReadHalfPosition);
        return true;
    }
    case EGame::GAME_HonorofKingsWorld:
        SerializeHonorofKingsWorldPositions(Ar, Stride, NumVertices, OutPositions);
        return true;
    case EGame::GAME_Farlight84: {
        bool bUseHalfPrecisionPositions = false;
        Ar << bUseHalfPrecisionPositions << Stride << NumVertices;
        if (!bUseHalfPrecisionPositions) {
            ReadFullPositions(Ar, OutPositions);
            return true;
        }
        Ar.SkipBytes(2 * 12);
        ReadPositions(Ar, 8, OutPositions, ReadHalfPosition);
        return true;
    }
    case EGame::GAME_SuicideSquad:
        SerializeSuicideSquadPositions(Ar, Stride, NumVertices, OutPositions);
        return true;
    default:
        return false;
    }
}

bool SerializeGameStridePositions(FArchive& Ar, uint32 Stride, TArray<FVector3f>& OutPositions) {
    switch (Ar.GetGame()) {
    case EGame::GAME_Valorant_PRE_11_2: {
        bool bUseFullPrecisionPositions = false;
        Ar << bUseFullPrecisionPositions;
        const FVector3f Origin = ReadFloatPosition(Ar);
        const FVector3f Extent = ReadFloatPosition(Ar);
        Ar.SkipBytes(4);
        if (bUseFullPrecisionPositions)
            return false;
        ReadPositions(Ar, 8, OutPositions, [&](FArchive& InAr) {
            const FVector3f Packed = ReadSignedShortPosition(InAr);
            return FVector3f(Packed.X * Extent.X + Origin.X, Packed.Y * Extent.Y + Origin.Y, Packed.Z * Extent.Z + Origin.Z);
        });
        return true;
    }
    case EGame::GAME_Gothic1Remake:
        if (Stride != 8)
            return false;
        ReadPositions(Ar, 8, OutPositions, ReadScaledHalfPosition);
        return true;
    case EGame::GAME_DaysGone:
        if (Stride == 4)
            ReadPositions(Ar, 4, OutPositions, ReadPacked32Position);
        else if (Stride == 8)
            ReadPositions(Ar, 8, OutPositions, ReadUnsignedShortPosition);
        else if (Stride == 12)
            ReadFullPositions(Ar, OutPositions);
        else
            Ar.SetError(ErrorCode::InvalidData, "FPositionVertexBuffer.Stride");
        return true;
    case EGame::GAME_RocoKingdomWorld:
        if (Stride == 8)
            ReadPositions(Ar, 8, OutPositions, ReadSignedShortPosition);
        else if (Stride == 12)
            ReadFullPositions(Ar, OutPositions);
        else
            Ar.SetError(ErrorCode::InvalidData, "FPositionVertexBuffer.Stride");
        return true;
    case EGame::GAME_FateTrigger: {
        uint8 bHasBox = 0;
        Ar << bHasBox;
        ReadFullPositions(Ar, OutPositions);
        if (bHasBox == 0)
            return true;
        Ar.SkipBytes(24);
        SkipBulkSerializedArray(Ar);
        return true;
    }
    default:
        return false;
    }
}

}

void FPositionVertexBuffer::Serialize(FArchive& Ar) {
    if (SerializeGamePositions(Ar, Stride, NumVertices, VertexData))
        return;
    Ar << Stride << NumVertices;
    if (SerializeGameStridePositions(Ar, Stride, VertexData))
        return;
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty) {
        Stride ^= 0xDBB1054F;
        NumVertices >>= 9;
    }
    if (Ar.GetGame() == EGame::GAME_Gollum)
        Ar.SkipBytes(25);
    ReadFullPositions(Ar, VertexData);
}

namespace {

FMeshTangents ReadTangents(FArchive& Ar, bool bHighPrecision) {
    FMeshTangents Tangent;
    if (bHighPrecision) {
        FPackedRGBA16N X, Z;
        Ar << X.X << X.Y << X.Z << X.W << Z.X << Z.Y << Z.Z << Z.W;
        Tangent.TangentX = X.ToFVector4f();
        Tangent.TangentZ = Z.ToFVector4f();
        return Tangent;
    }
    FPackedNormal X, Z;
    Ar << X.Packed << Z.Packed;
    Tangent.TangentX = X.ToFVector4f();
    Tangent.TangentZ = Z.ToFVector4f();
    return Tangent;
}

FVector2f ReadUV(FArchive& Ar, bool bFullPrecision) {
    FVector2f UV;
    if (bFullPrecision) {
        Ar << UV.X << UV.Y;
        return UV;
    }
    uint16 U = 0, V = 0;
    Ar << U << V;
    return FVector2f(HalfToFloat(U), HalfToFloat(V));
}

}

void FStaticMeshVertexBuffer::SerializeInterleaved(FArchive& Ar) {
    const int32 TangentSize = bUseHighPrecisionTangentBasis ? 16 : 8;
    const int32 Stride = TangentSize + static_cast<int32>(NumTexCoords) * (bUseFullPrecisionUVs ? 8 : 4);
    int32 Count = 0;
    if (!ReadBulkHeader(Ar, Stride, Count))
        return;
    if (static_cast<uint32>(Count) != NumVertices) {
        Ar.SetError(ErrorCode::InvalidData, "FStaticMeshVertexBuffer.Vertices");
        return;
    }
    Tangents.Empty(Count);
    TexCoords.Empty(Count * static_cast<int32>(NumTexCoords));
    for (int32 Vertex = 0; Vertex < Count && !Ar.IsError(); Vertex++) {
        Tangents.Add(ReadTangents(Ar, bUseHighPrecisionTangentBasis));
        for (uint32 Channel = 0; Channel < NumTexCoords; Channel++)
            TexCoords.Add(ReadUV(Ar, bUseFullPrecisionUVs));
    }
}

void FStaticMeshVertexBuffer::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    const bool bInterleaved = Ar.GetGame() < EGame::GAME_UE4_19;
    Ar << NumTexCoords;
    if (bInterleaved) {
        uint32 Stride = 0;
        Ar << Stride;
    }
    Ar << NumVertices << bUseFullPrecisionUVs;
    const EGame Game = Ar.GetGame();
    if (Game >= EGame::GAME_UE4_12)
        Ar << bUseHighPrecisionTangentBasis;
    if (Game == EGame::GAME_DeltaForce || Game == EGame::GAME_SuicideSquad)
        Ar.SkipBytes(4);
    int32 FateTriggerData = 0;
    if (Game == EGame::GAME_FateTrigger)
        Ar << FateTriggerData;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped())
        return;
    if (NumTexCoords > 8) {
        Ar.SetError(ErrorCode::InvalidData, "FStaticMeshVertexBuffer.NumTexCoords");
        return;
    }
    if (bInterleaved) {
        SerializeInterleaved(Ar);
        return;
    }
    bool bDropNormals = false;
    if (Game == EGame::GAME_StarWarsJediFallenOrder || Game == EGame::GAME_StarWarsJediSurvivor)
        Ar << bDropNormals;
    //! Honor of Kings: World packs its tangents in a format that is not known.
    if (Game == EGame::GAME_HonorofKingsWorld) {
        SkipBulkSerializedArray(Ar);
        bDropNormals = true;
    }
    if (bDropNormals) {
        Tangents.Empty(static_cast<int32>(NumVertices));
        Tangents.AddDefaulted(static_cast<int32>(NumVertices));
    } else {
        const int32 TangentSize = bUseHighPrecisionTangentBasis ? 16 : 8;
        int32 TangentCount = 0;
        if (!ReadBulkHeader(Ar, TangentSize, TangentCount))
            return;
        if (static_cast<uint32>(TangentCount) != NumVertices) {
            Ar.SetError(ErrorCode::InvalidData, "FStaticMeshVertexBuffer.Tangents");
            return;
        }
        Tangents.Empty(TangentCount);
        for (int32 Vertex = 0; Vertex < TangentCount; Vertex++)
            Tangents.Add(ReadTangents(Ar, bUseHighPrecisionTangentBasis));
        if (Game == EGame::GAME_FateTrigger && FateTriggerData > 0)
            SkipBulkSerializedArray(Ar);
    }
    const int32 UVSize = bUseFullPrecisionUVs ? 8 : 4;
    int32 UVCount = 0;
    if (!ReadBulkHeader(Ar, UVSize, UVCount))
        return;
    int64 TexCoordVertices = NumVertices;
    if (static_cast<int64>(UVCount) != static_cast<int64>(NumVertices) * NumTexCoords)
        TexCoordVertices = NumVertices + (NumVertices > 0 ? NumTexCoords % 2 : 0);
    if (static_cast<int64>(UVCount) != TexCoordVertices * NumTexCoords) {
        Ar.SetError(ErrorCode::InvalidData, "FStaticMeshVertexBuffer.TexCoords");
        return;
    }
    TexCoords.Empty(static_cast<int32>(NumVertices * NumTexCoords));
    for (int64 Vertex = 0; Vertex < TexCoordVertices; Vertex++) {
        for (uint32 Channel = 0; Channel < NumTexCoords; Channel++) {
            const FVector2f UV = ReadUV(Ar, bUseFullPrecisionUVs);
            if (Vertex < NumVertices)
                TexCoords.Add(UV);
        }
    }
    if (Game == EGame::GAME_TorchlightInfinite)
        SkipBulkSerializedArray(Ar);
}

void FPositionVertexBuffer::Init(TArray<FVector3f>&& InVertexData) {
    Stride = sizeof(FVector3f);
    NumVertices = static_cast<uint32>(InVertexData.Num());
    VertexData = std::move(InVertexData);
}

void FStaticMeshVertexBuffer::Init(TArray<FMeshTangents>&& InTangents, TArray<FVector2f>&& InTexCoords, uint32 InNumTexCoords, bool bInUseFullPrecisionUVs) {
    NumTexCoords = InNumTexCoords;
    NumVertices = static_cast<uint32>(InTangents.Num());
    bUseFullPrecisionUVs = bInUseFullPrecisionUVs;
    bUseHighPrecisionTangentBasis = false;
    Tangents = std::move(InTangents);
    TexCoords = std::move(InTexCoords);
}

void FColorVertexBuffer::Init(TArray<FColor>&& InVertexData) {
    Stride = sizeof(FColor);
    NumVertices = static_cast<uint32>(InVertexData.Num());
    VertexData = std::move(InVertexData);
}

void FColorVertexBuffer::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    Ar << Stride << NumVertices;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped() || NumVertices == 0)
        return;
    int32 Count = 0;
    if (!ReadBulkHeader(Ar, sizeof(FColor), Count))
        return;
    SerializeRaw(Ar, VertexData, Count);
}

void FRawStaticIndexBuffer::Serialize(FArchive& Ar) {
    const bool bByteBulk = Ar.UEVer() >= EUnrealEngineObjectUE4Version::SUPPORT_32BIT_STATIC_MESH_INDICES;
    if (bByteBulk)
        Ar << b32Bit;
    const int32 BulkElementSize = bByteBulk ? 1 : 2;
    int32 BulkCount = 0;
    if (!ReadBulkHeader(Ar, BulkElementSize, BulkCount))
        return;
    const int32 Size = BulkCount * BulkElementSize;
    std::span<const std::byte> Bytes = Ar.ReadView(Size);
    if (Ar.GetGame() >= EGame::GAME_UE4_25 && Ar.GetGame() != EGame::GAME_DeltaForce) {
        bool bShouldExpandTo32Bit = false;
        Ar << bShouldExpandTo32Bit;
    }
    if (Ar.IsError())
        return;
    const int32 ElementSize = b32Bit ? 4 : 2;
    const int32 Count = Size / ElementSize;
    Indices.Empty(Count);
    Indices.AddUninitialized(Count);
    for (int32 i = 0; i < Count; i++) {
        if (b32Bit) {
            uint32 Value;
            std::memcpy(&Value, Bytes.data() + static_cast<std::size_t>(i) * 4, 4);
            Indices[i] = Value;
        } else {
            uint16 Value;
            std::memcpy(&Value, Bytes.data() + static_cast<std::size_t>(i) * 2, 2);
            Indices[i] = Value;
        }
    }
    //! PUBG stores each index as a signed delta to the previous index.
    if (Ar.GetGame() != EGame::GAME_PlayerUnknownsBattlegrounds)
        return;
    int32 Current = 0;
    for (int32 i = 0; i < Count; i++) {
        Current += static_cast<int16>(Indices[i]);
        Indices[i] = static_cast<uint16>(Current);
    }
}

}
