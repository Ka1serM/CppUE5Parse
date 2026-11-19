#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/PackedNormal.h"

#include "Model.generated.h"

namespace ue5parse::runtime {

class ABrush;
class UMaterialInterface;

struct FBspNode {
    FPlane4f Plane;
    int32 iVertPool = 0;
    int32 iSurf = 0;
    int32 iVertexIndex = 0;
    uint16 ComponentIndex = 0;
    uint16 ComponentNodeIndex = 0;
    int32 ComponentElementIndex = 0;
    int32 iBack = INDEX_NONE;
    int32 iFront = INDEX_NONE;
    int32 iPlane = INDEX_NONE;
    int32 iCollisionBound = INDEX_NONE;
    uint8 iZone[2] = {};
    uint8 NumVertices = 0;
    uint8 NodeFlags = 0;
    int32 iLeaf[2] = {INDEX_NONE, INDEX_NONE};
};

struct FVert {
    int32 pVertex = 0;
    int32 iSide = INDEX_NONE;
    FVector2f ShadowTexCoord;
    FVector2f BackfaceShadowTexCoord;
};

struct FBspSurf {
    TObjectPtr<UMaterialInterface> Material = nullptr;
    uint32 PolyFlags = 0;
    int32 pBase = 0;
    int32 vNormal = 0;
    int32 vTextureU = 0;
    int32 vTextureV = 0;
    int32 iBrushPoly = INDEX_NONE;
    TObjectPtr<ABrush> Actor = nullptr;
    FPlane4f Plane;
    float LightMapScale = 0;
    int32 iLightmassIndex = 0;

    friend FArchive& operator<<(FArchive& Ar, FBspSurf& Surf);
};

struct FDepecatedModelVertex {
    FVector3f Position;
    FPackedNormal TangentX;
    FPackedNormal TangentZ;
    FVector2f TexCoord;
    FVector2f ShadowTexCoord;
};

struct FModelVertex {
    FVector3f Position;
    FVector3f TangentX;
    FVector4f TangentZ;
    FVector2f TexCoord;
    FVector2f ShadowTexCoord;

    friend FArchive& operator<<(FArchive& Ar, FModelVertex& Vertex);
};

struct FModelVertexBuffer {
    TArray<FModelVertex> Vertices;

    friend FArchive& operator<<(FArchive& Ar, FModelVertexBuffer& Buffer);
};

UCLASS()
class UModel : public UObject {
    GENERATED_BODY()
public:
    FBoxSphereBounds Bounds;
    TArray<FVector3f> Vectors;
    TArray<FVector3f> Points;
    TArray<FBspNode> Nodes;
    TArray<FBspSurf> Surfs;
    TArray<FVert> Verts;
    int32 NumSharedSides = 0;
    bool RootOutside = true;
    bool Linked = false;
    uint32 NumUniqueVertices = 0;
    FModelVertexBuffer VertexBuffer;
    FGuid LightingGuid;
    TArray<FLightmassPrimitiveSettings> LightmassSettings;

    void Serialize(FArchive& Ar) override;
};

}
