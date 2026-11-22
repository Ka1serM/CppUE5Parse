#pragma once

#include <optional>

#include "Engine/Source/Runtime/Engine/EngineUtils.h"
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
    //! Game data that Blue Protocol, Infinity Nikki and Honor of Kings: World keep per section.
    std::optional<int32> CustomData;

    friend FArchive& operator<<(FArchive& Ar, FStaticMeshSection& Section);
};

struct FStaticMeshLODResources {
    TArray<FStaticMeshSection> Sections;
    std::optional<FBoxSphereBounds> SourceMeshBounds;
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
    void SerializeFinalFantasy7RebirthIndices(FArchive& Ar);
    void SerializeLegacyBuffers(FArchive& Ar, const FStripDataFlags& StripFlags);
};

struct FNaniteResources {
    int32 RootDataSize = 0;
    int32 NumPageStreamingStates = 0;
    uint32 NumInputTriangles = 0;
    uint32 NumInputVertices = 0;

    void Serialize(FArchive& Ar);
};

class FStaticMeshRenderData {
public:
    TArray<FStaticMeshLODResources> LODResources;
    uint8 NumInlinedLODs = 0;
    FNaniteResources NaniteResources;
    FBoxSphereBounds Bounds;
    bool bLODsShareStaticLighting = false;
    float ScreenSize[8] = {};

    void Serialize(FArchive& Ar, bool bStreamableLODs = false);
    bool HasValidNaniteData() const { return NaniteResources.NumPageStreamingStates > 0; }

private:
    void SerializeDistanceFields(FArchive& Ar);
    void SerializeStreamableLODs(FArchive& Ar);
    void SerializeUndawnLODs(FArchive& Ar);
};

void SkipWeightedRandomSampler(FArchive& Ar);
void SkipBulkSerializedArray(FArchive& Ar);
void SkipDistanceField5(FArchive& Ar);

}
