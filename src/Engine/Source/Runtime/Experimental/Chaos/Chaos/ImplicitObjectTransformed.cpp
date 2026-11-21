#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObjectTransformed.h"

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObject.h"

namespace ue5parse::runtime::Chaos {

void FImplicitObjectTransformed::Serialize(FChaosArchive& Ar) {
    Ar << Object;
    Ar << Transform;
    SerializeAsAABB(Ar, LocalBoundingBox);
}

}
