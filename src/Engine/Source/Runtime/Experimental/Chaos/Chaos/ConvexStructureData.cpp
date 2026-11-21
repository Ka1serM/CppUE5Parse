#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ConvexStructureData.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime::Chaos {

template <class T_INDEX>
void TConvexHalfEdgeStructureData<T_INDEX>::Serialize(FArchive& Ar) {
    Ar << Planes << HalfEdges << Vertices;
    if (Ar.CustomVerAtLeast(FPhysicsObjectVersion::GUID, FPhysicsObjectVersion::ChaosConvexHasUniqueEdgeSet, EGame::GAME_UE4_27))
        Ar << Edges;
}

void FConvexStructureData::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FPhysicsObjectVersion::GUID, FPhysicsObjectVersion::ChaosConvexUsesHalfEdges, EGame::GAME_UE4_27)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FConvexStructureData.LoadLegacyData");
        return;
    }
    Ar << IndexType;
    switch (IndexType) {
    case EIndexType::None:
        return;
    case EIndexType::Small:
        Data.emplace<FConvexHalfEdgeStructureDataS>().Serialize(Ar);
        return;
    case EIndexType::Medium:
        Data.emplace<FConvexHalfEdgeStructureDataM>().Serialize(Ar);
        return;
    case EIndexType::Large:
        Data.emplace<FConvexHalfEdgeStructureDataL>().Serialize(Ar);
        return;
    }
    Ar.SetError(ErrorCode::InvalidData, "FConvexStructureData.IndexType");
}

}
