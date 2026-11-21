#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionRenderData.h"

namespace ue5parse::runtime {

void FBoneMapVertexBuffer::Serialize(FArchive& Ar) {
    Ar << NumVertices;
    BulkSerialize(Ar, BoneMapData);
}

void FGeometryCollectionMeshResources::Serialize(FArchive& Ar) {
    IndexBuffer.Serialize(Ar);
    PositionVertexBuffer.Serialize(Ar);
    StaticMeshVertexBuffer.Serialize(Ar);
    ColorVertexBuffer.Serialize(Ar);
    BoneMapVertexBuffer.Serialize(Ar);
}

void FGeometryCollectionMeshDescription::Serialize(FArchive& Ar) {
    Ar << NumVertices << NumTriangles;
    if (Ar.GetGame() < EGame::GAME_UE5_7)
        Ar << PreSkinnedBounds;
    Ar << Sections << SectionsNoInternal << SubSections;
}

void FGeometryCollectionRenderData::Serialize(FArchive& Ar) {
    Ar << bHasMeshData << bHasNaniteData;
    if (bHasMeshData) {
        MeshResource.Serialize(Ar);
        MeshDescription.Serialize(Ar);
    }
    if (bHasNaniteData)
        NaniteResources.Serialize(Ar);
    if (Ar.GetGame() >= EGame::GAME_UE5_7)
        Ar << PreSkinnedBounds;
}

}
