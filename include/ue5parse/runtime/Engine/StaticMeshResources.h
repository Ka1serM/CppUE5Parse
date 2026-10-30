#pragma once

#include "ue5parse/runtime/Engine/RawIndexBuffer.h"
#include "ue5parse/runtime/Engine/Rendering/ColorVertexBuffer.h"
#include "ue5parse/runtime/Engine/Rendering/PositionVertexBuffer.h"
#include "ue5parse/runtime/Engine/Rendering/StaticMeshVertexBuffer.h"

// The render data of static meshes (UE's StaticMeshResources.h), as the cook writes it. Nanite pages, Lumen cards and
// distance fields are passed over.

namespace ue5parse::runtime {

struct FStaticMeshVertexBuffers {
    FPositionVertexBuffer PositionVertexBuffer;
    FStaticMeshVertexBuffer StaticMeshVertexBuffer;
    FColorVertexBuffer ColorVertexBuffer;
};

struct FStaticMeshSection {
    int32 MaterialIndex = 0;
    uint32 FirstIndex = 0;
    uint32 NumTriangles = 0;
    uint32 MinVertexIndex = 0;
    uint32 MaxVertexIndex = 0;
    bool bEnableCollision = false;
    bool bCastShadow = true;
    bool bForceOpaque = false;
    bool bVisibleInRayTracing = true;
    bool bAffectDistanceFieldLighting = true;

    friend FArchive& operator<<(FArchive& Ar, FStaticMeshSection& Section);
};

struct FStaticMeshLODResources {
    TArray<FStaticMeshSection> Sections;
    float MaxDeviation = 0;
    bool bIsLODCookedOut = false;
    bool bBuffersInlined = false;
    // Whether the vertex and index buffers were read (a streamed LOD whose chunk is missing has none).
    bool bHasBuffers = false;
    FStaticMeshVertexBuffers VertexBuffers;
    FRawStaticIndexBuffer IndexBuffer;

    void Serialize(FArchive& Ar);
    int32 GetNumVertices() const { return static_cast<int32>(VertexBuffers.PositionVertexBuffer.GetNumVertices()); }
    int32 GetNumTriangles() const;
    int32 GetNumTexCoords() const { return static_cast<int32>(VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()); }

private:
    void SerializeBuffers(FArchive& Ar);
};

class FStaticMeshRenderData {
public:
    TArray<FStaticMeshLODResources> LODResources;
    uint8 NumInlinedLODs = 0;
    bool bHasNaniteData = false;
    FBoxSphereBounds Bounds;
    bool bLODsShareStaticLighting = false;
    float ScreenSize[8] = {};

    void Serialize(FArchive& Ar);
};

// Reads past FNaniteResources and tells whether the mesh had Nanite pages.
bool SerializeNaniteResources(FArchive& Ar);
// Reads past the cooked FWeightedRandomSampler of a section or LOD.
void SkipWeightedRandomSampler(FArchive& Ar);
// Reads past a BulkSerialize'd array (element size, count, elements).
void SkipBulkSerializedArray(FArchive& Ar);

}
