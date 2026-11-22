#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObjectScaled.h"

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObject.h"

namespace ue5parse::runtime::Chaos {

void FImplicitObjectScaled::Serialize(FChaosArchive& Ar) {
    Ar << Object;
    SerializeReal(Ar, Scale);
    SerializeReal(Ar, InvScale);
    SerializeAsAABB(Ar, LocalBoundingBox);
}

}
