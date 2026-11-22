#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Convex.h"

#include <cmath>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime::Chaos {

namespace {

bool UsesVerticesArray(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FPhysicsObjectVersion::GUID, FPhysicsObjectVersion::ConvexUsesVerticesArray, EGame::GAME_UE4_27)
        || Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::ConvexUsesVerticesArray, EGame::GAME_UE5_0)
        || Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::ChaosConvexVariableStructureDataAndVerticesArray,
            EGame::GAME_UE4_27);
}

}

void FConvex::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::ConvexUsesTPlaneConcrete, EGame::GAME_UE4_25)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FConvex.Planes");
        return;
    }
    Ar << Planes;
    if (!UsesVerticesArray(Ar)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FConvex.SurfaceParticles");
        return;
    }
    Ar << Vertices;
    SerializeAsAABB(Ar, LocalBoundingBox);
    if (Ar.CustomVerAtLeast(FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::AddConvexCenterOfMassAndVolume, EGame::GAME_UE4_25)) {
        Ar << Volume;
        //! Geometry collections store NaN for the volume of an empty convex; UE loads it as zero.
        if (std::isnan(Volume))
            Volume = 0.0f;
        Ar << CenterOfMass;
    }
    if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::MarginAddedToConvexAndBox, EGame::GAME_UE4_26))
        Ar << Margin;
    if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::StructureDataAddedToConvex, EGame::GAME_UE4_26))
        StructureData.Serialize(Ar);
    if (Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::AddedInertiaTensorAndRotationOfMassAddedToConvex,
            EGame::GAME_UE5_0))
        Ar << UnitMassInertiaTensor << RotationOfMass;
}

}
