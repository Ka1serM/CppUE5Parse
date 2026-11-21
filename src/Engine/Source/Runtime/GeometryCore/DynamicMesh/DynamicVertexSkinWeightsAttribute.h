#pragma once

#include "Engine/Source/Runtime/AnimationCore/BoneWeights.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FDynamicMeshVertexSkinWeightsAttribute {
public:
    void Serialize(FArchive& Ar);

    const TArray<TArray<FBoneWeight>>& GetVertexBoneWeights() const { return VertexBoneWeights; }

private:
    void SerializeUncompressed(FArchive& Ar);
    void SerializeCompressed(FArchive& Ar);

    TArray<TArray<FBoneWeight>> VertexBoneWeights;
};

}
