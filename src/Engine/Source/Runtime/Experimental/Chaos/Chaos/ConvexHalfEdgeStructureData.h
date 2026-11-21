#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime::Chaos {

template <class T_INDEX>
class TConvexHalfEdgeStructureData {
public:
    using FIndex = T_INDEX;

    struct FPlaneData {
        FIndex FirstHalfEdgeIndex = 0;
        FIndex NumHalfEdges = 0;

        friend FArchive& operator<<(FArchive& Ar, FPlaneData& Value) { return Ar << Value.FirstHalfEdgeIndex << Value.NumHalfEdges; }
    };

    struct FHalfEdgeData {
        FIndex PlaneIndex = 0;
        FIndex VertexIndex = 0;
        FIndex TwinHalfEdgeIndex = 0;

        friend FArchive& operator<<(FArchive& Ar, FHalfEdgeData& Value) { return Ar << Value.PlaneIndex << Value.VertexIndex << Value.TwinHalfEdgeIndex; }
    };

    struct FVertexData {
        FIndex FirstHalfEdgeIndex = 0;

        friend FArchive& operator<<(FArchive& Ar, FVertexData& Value) { return Ar << Value.FirstHalfEdgeIndex; }
    };

    void Serialize(FArchive& Ar);

    const TArray<FPlaneData>& GetPlanes() const { return Planes; }
    const TArray<FHalfEdgeData>& GetHalfEdges() const { return HalfEdges; }
    const TArray<FVertexData>& GetVertices() const { return Vertices; }
    const TArray<FIndex>& GetEdges() const { return Edges; }

private:
    TArray<FPlaneData> Planes;
    TArray<FHalfEdgeData> HalfEdges;
    TArray<FVertexData> Vertices;
    TArray<FIndex> Edges;
};

}
