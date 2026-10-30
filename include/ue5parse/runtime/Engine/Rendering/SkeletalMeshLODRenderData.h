#pragma once

#include "ue5parse/runtime/AnimationCore/BoneIndices.h"
#include "ue5parse/runtime/Engine/Rendering/MultiSizeIndexContainer.h"
#include "ue5parse/runtime/Engine/Rendering/SkinWeightVertexBuffer.h"
#include "ue5parse/runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

class USkeletalMesh;

// FSkelMeshRenderSection: a section of a skeletal mesh LOD and the bones it uses.
struct FSkelMeshRenderSection {
    uint16 MaterialIndex = 0;
    uint32 BaseIndex = 0;
    uint32 NumTriangles = 0;
    bool bRecomputeTangent = false;
    uint8 RecomputeTangentsVertexMaskChannel = 0;
    bool bCastShadow = true;
    bool bVisibleInRayTracing = true;
    uint32 BaseVertexIndex = 0;
    // Whether ClothMappingDataLODs has entries.
    bool bHasClothingData = false;
    TArray<FBoneIndexType> BoneMap;
    uint32 NumVertices = 0;
    int32 MaxBoneInfluences = 4;
    int16 CorrespondClothAssetIndex = INDEX_NONE;
    FGuid ClothingDataAssetGuid;
    int32 ClothingDataAssetLODIndex = 0;
    bool bDisabled = false;

    bool HasClothingData() const { return bHasClothingData; }
    friend FArchive& operator<<(FArchive& Ar, FSkelMeshRenderSection& Section);
};

// FSkeletalMeshLODRenderData: one LOD: sections, the bones it needs, and its buffers (inline or in bulk data).
class FSkeletalMeshLODRenderData {
public:
    TArray<FSkelMeshRenderSection> RenderSections;
    TArray<FBoneIndexType> RequiredBones;
    TArray<FBoneIndexType> ActiveBoneIndices;
    bool bIsLODOptional = false;
    bool bStreamedDataInlined = false;
    // Whether the buffers were read (a streamed LOD whose chunk is missing has none).
    bool bHasBuffers = false;
    FMultiSizeIndexContainer MultiSizeIndexContainer;
    FStaticMeshVertexBuffers StaticVertexBuffers;
    FSkinWeightVertexBuffer SkinWeightVertexBuffer;

    void Serialize(FArchive& Ar, bool bHasVertexColors);
    uint32 GetNumVertices() const { return StaticVertexBuffers.PositionVertexBuffer.GetNumVertices(); }
    uint32 GetNumTexCoords() const { return StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(); }
    int32 GetTotalFaces() const;
    // The section that holds a vertex, and the vertex's index in it.
    void GetSectionFromVertexIndex(int32 InVertIndex, int32& OutSectionIndex, int32& OutVertIndex) const;
    bool HasClothData() const;

private:
    void SerializeStreamedData(FArchive& Ar, bool bHasVertexColors);
    void SerializeAvailabilityInfo(FArchive& Ar);
};

}
