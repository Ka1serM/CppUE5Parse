#include "Engine/Source/Runtime/Engine/PhysicsSerializer.h"

namespace ue5parse::runtime {

void UPhysicsSerializer::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.UEVer() < EUnrealEngineObjectUE4Version::BODYINSTANCE_BINARY_SERIALIZATION)
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (bCooked)
        BinaryFormatData.Serialize(Ar);
}

}
