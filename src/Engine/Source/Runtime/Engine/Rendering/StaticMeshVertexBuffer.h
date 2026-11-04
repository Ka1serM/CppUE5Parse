#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FMeshTangents {
    FVector4f TangentX;
    FVector4f TangentZ;
};

class FStaticMeshVertexBuffer {
public:
    void Serialize(FArchive& Ar);
    const FVector2f& GetVertexUV(uint32 VertexIndex, uint32 UVIndex) const {
        return TexCoords[static_cast<int32>(VertexIndex * NumTexCoords + UVIndex)];
    }
    FVector4f VertexTangentZ(uint32 VertexIndex) const { return Tangents[static_cast<int32>(VertexIndex)].TangentZ; }
    const TArray<FMeshTangents>& GetTangentData() const { return Tangents; }
    const TArray<FVector2f>& GetTexCoordData() const { return TexCoords; }
    uint32 GetNumTexCoords() const { return NumTexCoords; }
    uint32 GetNumVertices() const { return NumVertices; }
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
