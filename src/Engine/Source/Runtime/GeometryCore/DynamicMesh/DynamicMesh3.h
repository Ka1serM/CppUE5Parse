#pragma once

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/Source/Runtime/GeometryCore/IndexTypes.h"

namespace ue5parse::runtime {

class FDynamicMesh3 {
public:
    void Serialize(FArchive& Ar);

    const TDynamicVector<FVector3d>& GetVertices() const { return Vertices; }
    const TOptional<TDynamicVector<FVector3f>>& GetVertexNormals() const { return VertexNormals; }
    const TOptional<TDynamicVector<FVector3f>>& GetVertexColors() const { return VertexColors; }
    const TOptional<TDynamicVector<FVector2f>>& GetVertexUVs() const { return VertexUVs; }
    const TDynamicVector<FIndex3i>& GetTriangles() const { return Triangles; }
    const TOptional<TDynamicVector<int32>>& GetTriangleGroups() const { return TriangleGroups; }
    const TOptional<FDynamicMeshAttributeSet>& Attributes() const { return AttributeSet; }

private:
    TDynamicVector<FVector3d> Vertices;
    TOptional<TDynamicVector<FVector3f>> VertexNormals;
    TOptional<TDynamicVector<FVector3f>> VertexColors;
    TOptional<TDynamicVector<FVector2f>> VertexUVs;
    TDynamicVector<FIndex3i> Triangles;
    TOptional<TDynamicVector<int32>> TriangleGroups;
    int32 GroupIDCounter = 0;
    TOptional<FDynamicMeshAttributeSet> AttributeSet;
};

}
