#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicVertexSkinWeightsAttribute.h"

#include <cstring>

#include "Engine/Source/Runtime/GeometryCore/Util/DynamicVector.h"

namespace ue5parse::runtime {

void FDynamicMeshVertexSkinWeightsAttribute::Serialize(FArchive& Ar) {
    bool bUseCompression = false;
    Ar << bUseCompression;
    VertexBoneWeights.Empty();
    if (bUseCompression)
        SerializeCompressed(Ar);
    else
        SerializeUncompressed(Ar);
}

void FDynamicMeshVertexSkinWeightsAttribute::SerializeUncompressed(FArchive& Ar) {
    uint32 SerializeNum = 0;
    Ar << SerializeNum;
    if (SerializeNum == 0 || Ar.IsError())
        return;
    bool bUseCompressionForBulkSerialization = false;
    Ar << bUseCompressionForBulkSerialization;
    if (bUseCompressionForBulkSerialization || static_cast<int64>(SerializeNum) > Ar.GetRemaining() / static_cast<int64>(sizeof(int32))) {
        Ar.SetError(ErrorCode::InvalidData, "TDynamicVertexSkinWeightsAttribute.VertexBoneWeights");
        return;
    }
    for (uint32 Index = 0; Index < SerializeNum && !Ar.IsError(); Index++)
        Ar << VertexBoneWeights[VertexBoneWeights.AddDefaulted()];
}

void FDynamicMeshVertexSkinWeightsAttribute::SerializeCompressed(FArchive& Ar) {
    int32 BufferSize = 0;
    Ar << BufferSize;
    if (BufferSize < 1) {
        Ar.SetError(ErrorCode::InvalidData, "TDynamicVertexSkinWeightsAttribute.BufferSize");
        return;
    }
    TArray<int32> Buffer;
    Buffer.AddUninitialized(BufferSize);
    SerializeCompressedNew(Ar, std::span(reinterpret_cast<std::byte*>(Buffer.GetData()), static_cast<std::size_t>(BufferSize) * sizeof(int32)));
    if (Ar.IsError())
        return;
    int32 Cursor = 0;
    const int32 NumVertices = Buffer[Cursor++];
    for (int32 Vertex = 0; Vertex < NumVertices; Vertex++) {
        if (Cursor >= BufferSize || Buffer[Cursor] < 0 || Buffer[Cursor] > BufferSize - Cursor - 1) {
            Ar.SetError(ErrorCode::InvalidData, "TDynamicVertexSkinWeightsAttribute.Buffer");
            return;
        }
        const int32 Num = Buffer[Cursor++];
        TArray<FBoneWeight>& BoneWeights = VertexBoneWeights[VertexBoneWeights.AddDefaulted()];
        BoneWeights.AddUninitialized(Num);
        std::memcpy(BoneWeights.GetData(), Buffer.GetData() + Cursor, static_cast<std::size_t>(Num) * sizeof(FBoneWeight));
        Cursor += Num;
    }
    if (Cursor != BufferSize)
        Ar.SetError(ErrorCode::InvalidData, "TDynamicVertexSkinWeightsAttribute.BufferEnd");
}

}
