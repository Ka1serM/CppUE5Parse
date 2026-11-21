#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Capsule.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObject.h"

namespace ue5parse::runtime::Chaos {

void FCapsule::Serialize(FChaosArchive& Ar) {
    Ar << Segment << Radius;
    if (Ar.CustomVerAtLeast(FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::CapsulesNoUnionOrAABBs, EGame::GAME_UE4_25))
        return;
    FAABB3 LegacyBounds;
    SerializeAsAABB(Ar, LegacyBounds);
    FImplicitObjectPtr LegacyUnion;
    Ar << LegacyUnion;
}

}
