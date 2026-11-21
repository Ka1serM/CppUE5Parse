#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicMesh3.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

template <class Type>
void SerializeOptionalVector(FArchive& Ar, TOptional<Type>& OptionalVector) {
    bool bHasOptionalVector = false;
    Ar << bHasOptionalVector;
    OptionalVector.Reset();
    if (bHasOptionalVector)
        OptionalVector.Emplace().Serialize(Ar);
}

}

void FDynamicMesh3::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::DynamicMeshCompactedSerialization, EGame::GAME_UE5_0)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FDynamicMesh3.InitialVersion");
        return;
    }
    bool bPreserveDataLayout = false;
    bool bCompactData = false;
    bool bUseCompression = false;
    Ar << bPreserveDataLayout << bCompactData << bUseCompression;
    //! Only transaction archives write the variant without bCompactData.
    if (!bCompactData) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FDynamicMesh3.Default");
        return;
    }
    Vertices.Serialize(Ar);
    SerializeOptionalVector(Ar, VertexNormals);
    SerializeOptionalVector(Ar, VertexColors);
    SerializeOptionalVector(Ar, VertexUVs);
    Triangles.Serialize(Ar);
    SerializeOptionalVector(Ar, TriangleGroups);
    Ar << GroupIDCounter;
    bool bHasAttributes = false;
    Ar << bHasAttributes;
    AttributeSet.Reset();
    if (bHasAttributes)
        AttributeSet.Emplace().Serialize(Ar);
}

}
