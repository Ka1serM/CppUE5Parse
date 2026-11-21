#pragma once

#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicVertexAttribute.h"

namespace ue5parse::runtime {

class FDynamicMeshSculptLayers {
public:
    void Serialize(FArchive& Ar, int32 NumLayers) {
        Ar << ActiveLayer << LayerWeights;
        Layers.Empty(NumLayers);
        for (int32 LayerIdx = 0; LayerIdx < NumLayers && !Ar.IsError(); LayerIdx++)
            Layers[Layers.AddDefaulted()].Serialize(Ar);
    }

    int32 NumLayers() const { return Layers.Num(); }
    int32 GetActiveLayer() const { return ActiveLayer; }
    const TArray<double>& GetLayerWeights() const { return LayerWeights; }

private:
    TArray<FDynamicMeshSculptLayerAttribute> Layers;
    TArray<double> LayerWeights;
    int32 ActiveLayer = -1;
};

}
