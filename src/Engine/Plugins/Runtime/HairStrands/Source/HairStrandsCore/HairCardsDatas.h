#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Engine/PackedNormal.h"

namespace ue5parse::runtime {

class FArchive;

struct FHairCardsBulkData {
    TArray<FVector4f> Positions;
    TArray<FPackedNormal> Normals;
    TArray<FVector4f> UVs;
    TArray<uint32> Materials;
    TArray<uint32> VertexColors;
    TArray<uint32> Indices;
    FBox BoundingBox;
    uint32 NumUVs = 0;
    bool bVertexColor = false;

    void Serialize(FArchive& Ar);
};

struct FHairCardsInterpolationVertex {
    uint32 VertexIndex : 24 = 0;
    uint32 VertexLerp : 8 = 0;
};

struct FHairCardsInterpolationBulkData {
    TArray<FHairCardsInterpolationVertex> Interpolation;

    void Serialize(FArchive& Ar);
};

struct FHairMeshesBulkData {
    TArray<FVector4f> Positions;
    TArray<FPackedNormal> Normals;
    TArray<FVector4f> UVs;
    TArray<uint32> VertexColors;
    TArray<uint32> Indices;
    FBox BoundingBox;
    uint32 NumUVs = 0;
    bool bVertexColor = false;

    void Serialize(FArchive& Ar);
};

}
