#pragma once

#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

// A vertex's tangent basis, unpacked from the 8-bit or 16-bit packed values: TangentX and TangentZ (the normal; W is the
// sign of the binormal). Kept at full precision.
struct FMeshTangents {
    FVector4f TangentX;
    FVector4f TangentZ;
};

// FStaticMeshVertexBuffer: tangents and texture coordinates. TexCoords holds NumTexCoords entries per vertex, unpacked
// from half precision when the mesh stores them so.
class FStaticMeshVertexBuffer {
public:
    void Serialize(FArchive& Ar);
    const FVector2f& GetVertexUV(uint32 VertexIndex, uint32 UVIndex) const {
        return TexCoords[static_cast<int32>(VertexIndex * NumTexCoords + UVIndex)];
    }
    FVector4f VertexTangentX(uint32 VertexIndex) const { return Tangents[static_cast<int32>(VertexIndex)].TangentX; }
    FVector4f VertexTangentZ(uint32 VertexIndex) const { return Tangents[static_cast<int32>(VertexIndex)].TangentZ; }
    // The binormal from the normal, the tangent and the sign in TangentZ.W (UE's VertexTangentY).
    FVector3f VertexTangentY(uint32 VertexIndex) const;
    uint32 GetNumTexCoords() const { return NumTexCoords; }
    uint32 GetNumVertices() const { return NumVertices; }
    bool GetUseFullPrecisionUVs() const { return bUseFullPrecisionUVs; }
    bool GetUseHighPrecisionTangentBasis() const { return bUseHighPrecisionTangentBasis; }
    bool HasData() const { return Tangents.Num() > 0; }

private:
    uint32 NumTexCoords = 0;
    uint32 NumVertices = 0;
    bool bUseFullPrecisionUVs = true;
    bool bUseHighPrecisionTangentBasis = false;
    TArray<FMeshTangents> Tangents;
    TArray<FVector2f> TexCoords;
};

}
