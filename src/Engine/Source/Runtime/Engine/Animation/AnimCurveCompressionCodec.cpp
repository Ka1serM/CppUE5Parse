#include "Engine/Source/Runtime/Engine/Animation/AnimCurveCompressionCodec.h"

namespace ue5parse::runtime {

void UAnimCurveCompressionCodec::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    if (Ar.GetRemaining() >= 16) Ar << InstanceGuid;
}

}
