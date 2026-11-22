#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FPositionVertexBuffer {
public:
    void Serialize(FArchive& Ar);
    void Init(TArray<FVector3f>&& InVertexData);
    uint32 GetNumVertices() const { return NumVertices; }
    uint32 GetStride() const { return Stride; }
    const TArray<FVector3f>& GetVertexData() const { return VertexData; }

private:
    uint32 Stride = 0;
    uint32 NumVertices = 0;
    TArray<FVector3f> VertexData;
};

}
