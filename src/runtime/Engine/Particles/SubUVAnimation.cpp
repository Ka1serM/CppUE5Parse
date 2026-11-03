#include "ue5parse/runtime/Engine/Particles/SubUVAnimation.h"

// UE's Particles/SubUVAnimation.cpp (UE 5.3).

namespace ue5parse::runtime {

void USubUVAnimation::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked) Ar << BoundingGeometry;
}

}
