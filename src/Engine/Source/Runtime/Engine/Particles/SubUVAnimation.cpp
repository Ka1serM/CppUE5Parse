#include "Engine/Source/Runtime/Engine/Particles/SubUVAnimation.h"

namespace ue5parse::runtime {

void FSubUVDerivedData::Serialize(FArchive& Ar) {
    Ar << BoundingGeometry;
}

void USubUVAnimation::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        DerivedData.Serialize(Ar);
}

}
