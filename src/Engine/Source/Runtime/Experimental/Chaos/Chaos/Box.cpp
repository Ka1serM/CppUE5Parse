#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Box.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime::Chaos {

void FImplicitBox3::Serialize(FArchive& Ar) {
    AABB.Serialize(Ar);
    if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::MarginAddedToConvexAndBox, EGame::GAME_UE4_26))
        Ar << Margin;
}

}
