#pragma once

#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

// FPositionVertexBuffer: the vertex positions of a mesh LOD.
class FPositionVertexBuffer {
public:
    void Serialize(FArchive& Ar);
    const FVector3f& VertexPosition(uint32 VertexIndex) const { return VertexData[static_cast<int32>(VertexIndex)]; }
    uint32 GetNumVertices() const { return NumVertices; }
    uint32 GetStride() const { return Stride; }
    const TArray<FVector3f>& GetVertexData() const { return VertexData; }

private:
    uint32 Stride = 0;
    uint32 NumVertices = 0;
    TArray<FVector3f> VertexData;
};

}
