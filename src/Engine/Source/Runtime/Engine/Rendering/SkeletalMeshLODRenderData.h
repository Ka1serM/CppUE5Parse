#pragma once

#include "Engine/Source/Runtime/AnimationCore/BoneIndices.h"
#include "Engine/Source/Runtime/Engine/Rendering/MultiSizeIndexContainer.h"
#include "Engine/Source/Runtime/Engine/Rendering/SkinWeightVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

class USkeletalMesh;

struct FSkelMeshRenderSection {
    uint16 MaterialIndex = 0;
    uint32 BaseIndex = 0;
    uint32 NumTriangles = 0;
    bool bRecomputeTangent = false;
    uint8 RecomputeTangentsVertexMaskChannel = 0;
    bool bCastShadow = true;
    bool bVisibleInRayTracing = true;
    uint32 BaseVertexIndex = 0;
    bool bHasClothingData = false;
    TArray<FBoneIndexType> BoneMap;
    uint32 NumVertices = 0;
    int32 MaxBoneInfluences = 4;
    int16 CorrespondClothAssetIndex = INDEX_NONE;
    FGuid ClothingDataAssetGuid;
    int32 ClothingDataAssetLODIndex = 0;
    bool bDisabled = false;

    friend FArchive& operator<<(FArchive& Ar, FSkelMeshRenderSection& Section);
};

class FSkeletalMeshLODRenderData {
public:
    TArray<FSkelMeshRenderSection> RenderSections;
    TArray<FBoneIndexType> RequiredBones;
    TArray<FBoneIndexType> ActiveBoneIndices;
    bool bIsLODOptional = false;
    bool bStreamedDataInlined = false;
    bool bHasBuffers = false;
    FMultiSizeIndexContainer MultiSizeIndexContainer;
    FStaticMeshVertexBuffers StaticVertexBuffers;
    FSkinWeightVertexBuffer SkinWeightVertexBuffer;

    void Serialize(FArchive& Ar, bool bHasVertexColors);
    uint32 GetNumVertices() const { return StaticVertexBuffers.PositionVertexBuffer.GetNumVertices(); }
    uint32 GetNumTexCoords() const { return StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(); }
    bool HasClothData() const;

private:
    void SerializeStreamedData(FArchive& Ar, bool bHasVertexColors);
    void SerializeAvailabilityInfo(FArchive& Ar);
};

}
