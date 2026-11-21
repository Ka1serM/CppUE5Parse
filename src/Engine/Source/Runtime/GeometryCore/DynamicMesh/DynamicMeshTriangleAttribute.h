#pragma once

#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicAttribute.h"
#include "Engine/Source/Runtime/GeometryCore/Util/DynamicVector.h"

namespace ue5parse::runtime {

template <class AttribValueType, int32 AttribDimension>
class TDynamicMeshTriangleAttribute : public TDynamicAttributeBase {
public:
    void Serialize(FArchive& Ar) {
        TDynamicAttributeBase::Serialize(Ar);
        bool bUseCompression = false;
        Ar << bUseCompression;
        AttribValues.Serialize(Ar);
    }

    const TDynamicVector<AttribValueType>& GetValues() const { return AttribValues; }

private:
    TDynamicVector<AttribValueType> AttribValues;
};

using FDynamicMeshMaterialAttribute = TDynamicMeshTriangleAttribute<int32, 1>;
using FDynamicMeshPolygroupAttribute = TDynamicMeshTriangleAttribute<int32, 1>;

}
