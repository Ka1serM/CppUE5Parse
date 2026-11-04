#pragma once

#include "Engine/Source/Runtime/Engine/RawIndexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/ColorVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/PositionVertexBuffer.h"
#include "Engine/Source/Runtime/Engine/Rendering/StaticMeshVertexBuffer.h"

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
    bool bHasBuffers = false;
    FStaticMeshVertexBuffers VertexBuffers;
    FRawStaticIndexBuffer IndexBuffer;

    void Serialize(FArchive& Ar);
    int32 GetNumVertices() const { return static_cast<int32>(VertexBuffers.PositionVertexBuffer.GetNumVertices()); }
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

bool SerializeNaniteResources(FArchive& Ar);
void SkipWeightedRandomSampler(FArchive& Ar);
void SkipBulkSerializedArray(FArchive& Ar);

}
