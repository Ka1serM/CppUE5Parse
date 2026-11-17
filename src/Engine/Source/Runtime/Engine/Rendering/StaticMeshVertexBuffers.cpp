#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/PackedNormal.h"
#include "Engine/Source/Runtime/Engine/RawIndexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/ColorVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/PositionVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/StaticMeshVertexBuffer.h"

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

void FPositionVertexBuffer::Serialize(FArchive& Ar) {
    Ar << Stride << NumVertices;
    int32 Count = 0;
    if (!ReadBulkHeader(Ar, sizeof(FVector3f), Count))
        return;
    SerializeRaw(Ar, VertexData, Count);
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
    if (Ar.GetGame() >= EGame::GAME_UE4_12)
        Ar << bUseHighPrecisionTangentBasis;
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
    if (Ar.GetGame() >= EGame::GAME_UE4_25) {
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
}

}
