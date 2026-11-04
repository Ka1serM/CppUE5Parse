#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FColorVertexBuffer {
public:
    void Serialize(FArchive& Ar);
    uint32 GetNumVertices() const { return NumVertices; }
    const FColor& VertexColor(uint32 VertexIndex) const { return VertexData[static_cast<int32>(VertexIndex)]; }
    const TArray<FColor>& GetVertexData() const { return VertexData; }

private:
    uint32 Stride = 0;
    uint32 NumVertices = 0;
    TArray<FColor> VertexData;
};

}
