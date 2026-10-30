#include "ue5parse/runtime/Engine/Animation/AnimCurveCompressionCodec.h"

namespace ue5parse::runtime {

void UAnimCurveCompressionCodec::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    // UE 5.3 cooks the instance guid (SerializeAnimCurveCompressionCodecGuidOnCook, removed in 5.4).
    if (Ar.GetRemaining() >= 16) Ar << InstanceGuid;
}

}
