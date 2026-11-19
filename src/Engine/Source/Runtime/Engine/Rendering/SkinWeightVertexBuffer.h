#pragma once

#include <span>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FSkinWeightVertexBuffer {
public:
    void Serialize(FArchive& Ar);
    void SerializeMetaData(FArchive& Ar);
    void Init(TArray<uint16>&& InBoneIndices, TArray<uint16>&& InBoneWeights, uint32 InMaxBoneInfluences, uint32 InNumVertices);
    uint32 GetNumVertices() const { return NumVertices; }
    uint32 GetMaxBoneInfluences() const { return MaxBoneInfluences; }
    bool Use16BitBoneWeight() const { return bUse16BitBoneWeight; }
    uint32 GetBoneIndex(uint32 VertexIndex, uint32 InfluenceIndex) const {
        return BoneIndices[static_cast<int32>(VertexIndex * MaxBoneInfluences + InfluenceIndex)];
    }
    uint16 GetBoneWeight(uint32 VertexIndex, uint32 InfluenceIndex) const {
        return BoneWeights[static_cast<int32>(VertexIndex * MaxBoneInfluences + InfluenceIndex)];
    }
    bool HasData() const { return BoneIndices.Num() > 0; }

private:
    std::span<const std::byte> SerializeData(FArchive& Ar, TArray<uint32>& Lookup);
    std::span<const std::byte> SerializeLegacy(FArchive& Ar);
    void ReadWeights(FArchive& Ar, std::span<const std::byte> Data, const TArray<uint32>& Lookup);

    bool bVariableBonesPerVertex = false;
    uint32 MaxBoneInfluences = 0;
    uint32 NumBones = 0;
    uint32 NumVertices = 0;
    bool bUse16BitBoneIndex = false;
    bool bUse16BitBoneWeight = false;
    TArray<uint16> BoneIndices;
    TArray<uint16> BoneWeights;
};

}
