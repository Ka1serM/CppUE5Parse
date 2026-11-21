#pragma once

#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicAttribute.h"
#include "Engine/Source/Runtime/GeometryCore/Util/DynamicVector.h"

namespace ue5parse::runtime {

template <class AttribValueType, int32 AttribDimension>
class TDynamicMeshVertexAttribute : public TDynamicAttributeBase {
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

using FDynamicMeshWeightAttribute = TDynamicMeshVertexAttribute<float, 1>;
using FDynamicMeshMorphTargetAttribute = TDynamicMeshVertexAttribute<float, 3>;
using FDynamicMeshSculptLayerAttribute = TDynamicMeshVertexAttribute<double, 3>;

}
