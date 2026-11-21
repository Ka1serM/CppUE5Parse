#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicBoneAttribute.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicMeshOverlay.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicMeshSculptLayers.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicMeshTriangleAttribute.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicVertexAttribute.h"
#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicVertexSkinWeightsAttribute.h"

namespace ue5parse::runtime {

struct FDynamicMeshBones {
    FDynamicMeshBoneNameAttribute BoneNameAttrib;
    FDynamicMeshBoneParentIndexAttribute BoneParentIndexAttrib;
    FDynamicMeshBonePoseAttribute BonePoseAttrib;
    FDynamicMeshBoneColorAttribute BoneColorAttrib;
};

class FDynamicMeshTriangleLabelAttribute {
public:
    void Serialize(FArchive& Ar);

    const TArray<FName>& GetLabels() const { return Labels; }

private:
    TArray<FName> Labels;
};

class FDynamicMeshAttributeSet {
public:
    void Serialize(FArchive& Ar);

    const TArray<FDynamicMeshUVOverlay>& GetUVLayers() const { return UVLayers; }
    const TArray<FDynamicMeshNormalOverlay>& GetNormalLayers() const { return NormalLayers; }
    const TArray<FDynamicMeshPolygroupAttribute>& GetPolygroupLayers() const { return PolygroupLayers; }
    const TArray<FDynamicMeshWeightAttribute>& GetWeightLayers() const { return WeightLayers; }
    const TOptional<FDynamicMeshColorOverlay>& GetColorLayer() const { return ColorLayer; }
    const TOptional<FDynamicMeshMaterialAttribute>& GetMaterialID() const { return MaterialIDAttrib; }

private:
    template <class AttributeType>
    static void SerializeNamedAttributes(FArchive& Ar, TMap<FName, AttributeType>& Attributes);

    TArray<FDynamicMeshUVOverlay> UVLayers;
    TArray<FDynamicMeshNormalOverlay> NormalLayers;
    TArray<FDynamicMeshPolygroupAttribute> PolygroupLayers;
    TArray<FDynamicMeshWeightAttribute> WeightLayers;
    TOptional<FDynamicMeshColorOverlay> ColorLayer;
    TOptional<FDynamicMeshMaterialAttribute> MaterialIDAttrib;
    TMap<FName, FDynamicMeshVertexSkinWeightsAttribute> SkinWeightAttributes;
    TMap<FName, FDynamicMeshMorphTargetAttribute> MorphTargetAttributes;
    TOptional<FDynamicMeshBones> Bones;
    FDynamicMeshSculptLayers SculptLayers;
    TMap<FName, FDynamicMeshTriangleLabelAttribute> TriangleLabelAttributes;
};

}
