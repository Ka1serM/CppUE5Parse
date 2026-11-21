#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObjectUnion.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObject.h"

namespace ue5parse::runtime::Chaos {

void FImplicitObjectUnion::Serialize(FChaosArchive& Ar) {
    Ar << Objects;
    SerializeAsAABB(Ar, LocalBoundingBox);
    if (!Ar.CustomVerAtLeast(FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::UnionObjectsCanAvoidHierarchy, EGame::GAME_UE4_25)) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FImplicitObjectUnion.LegacyBVH");
        return;
    }
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::ChaosImplicitObjectUnionBVHRefactor, EGame::GAME_UE5_3)) {
        bool bHierarchyBuilt = false;
        Ar << bHierarchyBuilt;
        if (bHierarchyBuilt)
            Ar.SetError(ErrorCode::UnsupportedVersion, "FImplicitObjectUnion.LegacyBVH");
        return;
    }
    Ar << Flags;
    if (Ar.CustomVerAtLeast(FFortniteSeasonBranchObjectVersion::GUID, FFortniteSeasonBranchObjectVersion::ChaosImplicitObjectUnionLeafObjectsToInt32, EGame::GAME_UE5_4)) {
        Ar << NumLeafObjects;
    } else {
        uint16 NumLeafObjects16 = 0;
        Ar << NumLeafObjects16;
        NumLeafObjects = NumLeafObjects16;
    }
    if (Flags & Flag_HasBVH)
        Ar.SetError(ErrorCode::UnsupportedVersion, "FImplicitObjectUnion.BVH");
}

}
