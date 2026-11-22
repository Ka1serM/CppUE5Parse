#pragma once

#include "Engine/Source/Runtime/GeometryCore/Util/RefCountVector.h"

namespace ue5parse::runtime {

template <class RealType, int32 ElementSize>
class TDynamicMeshOverlay {
public:
    void Serialize(FArchive& Ar) {
        ElementsRefCounts.Serialize(Ar);
        Elements.Serialize(Ar);
        ParentVertices.Serialize(Ar);
        ElementTriangles.Serialize(Ar);
    }

    const TDynamicVector<RealType>& GetElements() const { return Elements; }
    const TDynamicVector<int32>& GetParentVertices() const { return ParentVertices; }
    const TDynamicVector<int32>& GetElementTriangles() const { return ElementTriangles; }

private:
    FRefCountVector ElementsRefCounts;
    TDynamicVector<RealType> Elements;
    TDynamicVector<int32> ParentVertices;
    TDynamicVector<int32> ElementTriangles;
};

using FDynamicMeshUVOverlay = TDynamicMeshOverlay<float, 2>;
using FDynamicMeshNormalOverlay = TDynamicMeshOverlay<float, 3>;
using FDynamicMeshColorOverlay = TDynamicMeshOverlay<float, 4>;

}
