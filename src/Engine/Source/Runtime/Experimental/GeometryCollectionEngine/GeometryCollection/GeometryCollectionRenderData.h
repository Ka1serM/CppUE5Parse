#pragma once

#include "Engine/Source/Runtime/Engine/RawIndexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/ColorVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/PositionVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/StaticMeshVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

class FBoneMapVertexBuffer {
public:
    void Serialize(FArchive& Ar);

    uint32 GetNumVertices() const { return NumVertices; }
    const TArray<uint16>& GetBoneMapData() const { return BoneMapData; }

private:
    uint32 NumVertices = 0;
    TArray<uint16> BoneMapData;
};

struct FGeometryCollectionMeshResources {
    FRawStaticIndexBuffer IndexBuffer;
    FPositionVertexBuffer PositionVertexBuffer;
    FStaticMeshVertexBuffer StaticMeshVertexBuffer;
    FColorVertexBuffer ColorVertexBuffer;
    FBoneMapVertexBuffer BoneMapVertexBuffer;

    void Serialize(FArchive& Ar);
};

struct FGeometryCollectionMeshElement {
    int16 TransformIndex = 0;
    uint8 MaterialIndex = 0;
    uint8 bIsInternal = 0;
    uint32 TriangleStart = 0;
    uint32 TriangleCount = 0;
    uint32 VertexStart = 0;
    uint32 VertexEnd = 0;

    friend FArchive& operator<<(FArchive& Ar, FGeometryCollectionMeshElement& Item) {
        return Ar << Item.TransformIndex << Item.MaterialIndex << Item.bIsInternal << Item.TriangleStart << Item.TriangleCount << Item.VertexStart << Item.VertexEnd;
    }
};

struct FGeometryCollectionMeshDescription {
    uint32 NumVertices = 0;
    uint32 NumTriangles = 0;
    FBoxSphereBounds PreSkinnedBounds;
    TArray<FGeometryCollectionMeshElement> Sections;
    TArray<FGeometryCollectionMeshElement> SectionsNoInternal;
    TArray<FGeometryCollectionMeshElement> SubSections;

    void Serialize(FArchive& Ar);
};

class FGeometryCollectionRenderData {
public:
    void Serialize(FArchive& Ar);

    bool HasMeshData() const { return bHasMeshData; }
    bool HasNaniteData() const { return bHasNaniteData; }
    const FGeometryCollectionMeshResources& GetMeshResource() const { return MeshResource; }
    const FGeometryCollectionMeshDescription& GetMeshDescription() const { return MeshDescription; }
    const FNaniteResources& GetNaniteResources() const { return NaniteResources; }
    const FBoxSphereBounds& GetPreSkinnedBounds() const { return PreSkinnedBounds; }

private:
    bool bHasMeshData = false;
    bool bHasNaniteData = false;
    FGeometryCollectionMeshResources MeshResource;
    FGeometryCollectionMeshDescription MeshDescription;
    FNaniteResources NaniteResources;
    FBoxSphereBounds PreSkinnedBounds;
};

}
