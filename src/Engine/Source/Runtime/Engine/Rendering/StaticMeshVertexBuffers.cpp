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
    if (Ar.IsError()) return false;
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
    if (!ReadBulkHeader(Ar, sizeof(FVector3f), Count)) return;
    SerializeRaw(Ar, VertexData, Count);
}

void FStaticMeshVertexBuffer::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    Ar << NumTexCoords << NumVertices << bUseFullPrecisionUVs << bUseHighPrecisionTangentBasis;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped()) return;
    if (NumTexCoords > 8) {
        Ar.SetError(ErrorCode::InvalidData, "FStaticMeshVertexBuffer.NumTexCoords");
        return;
    }
    const int32 TangentSize = bUseHighPrecisionTangentBasis ? 16 : 8;
    int32 TangentCount = 0;
    if (!ReadBulkHeader(Ar, TangentSize, TangentCount)) return;
    if (static_cast<uint32>(TangentCount) != NumVertices) {
        Ar.SetError(ErrorCode::InvalidData, "FStaticMeshVertexBuffer.Tangents");
        return;
    }
    Tangents.Empty(TangentCount);
    for (int32 Vertex = 0; Vertex < TangentCount; Vertex++) {
        FMeshTangents& Tangent = Tangents[Tangents.AddDefaulted()];
        if (bUseHighPrecisionTangentBasis) {
            FPackedRGBA16N X, Z;
            Ar << X.X << X.Y << X.Z << X.W << Z.X << Z.Y << Z.Z << Z.W;
            Tangent.TangentX = X.ToFVector4f();
            Tangent.TangentZ = Z.ToFVector4f();
        } else {
            FPackedNormal X, Z;
            Ar << X.Packed << Z.Packed;
            Tangent.TangentX = X.ToFVector4f();
            Tangent.TangentZ = Z.ToFVector4f();
        }
    }
    const int32 UVSize = bUseFullPrecisionUVs ? 8 : 4;
    int32 UVCount = 0;
    if (!ReadBulkHeader(Ar, UVSize, UVCount)) return;
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
            FVector2f UV;
            if (bUseFullPrecisionUVs) {
                Ar << UV.X << UV.Y;
            } else {
                uint16 U = 0, V = 0;
                Ar << U << V;
                UV = FVector2f(HalfToFloat(U), HalfToFloat(V));
            }
            if (Vertex < NumVertices) TexCoords.Add(UV);
        }
    }
}

void FColorVertexBuffer::Serialize(FArchive& Ar) {
    FStripDataFlags StripFlags(Ar);
    Ar << Stride << NumVertices;
    if (Ar.IsError() || StripFlags.IsAudioVisualDataStripped() || NumVertices == 0) return;
    int32 Count = 0;
    if (!ReadBulkHeader(Ar, sizeof(FColor), Count)) return;
    SerializeRaw(Ar, VertexData, Count);
}

void FRawStaticIndexBuffer::Serialize(FArchive& Ar) {
    Ar << b32Bit;
    int32 Size = 0;
    if (!ReadBulkHeader(Ar, 1, Size)) return;
    std::span<const std::byte> Bytes = Ar.ReadView(Size);
    bool bShouldExpandTo32Bit = false;
    Ar << bShouldExpandTo32Bit;
    if (Ar.IsError()) return;
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
