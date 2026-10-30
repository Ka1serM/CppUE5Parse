#include "ue5parse/runtime/Engine/Animation/AnimCurveCompressionSettings.h"

#include "ue5parse/runtime/Engine/Animation/AnimCurveCompressionCodec_CompressedRichCurve.h"

namespace ue5parse::runtime {

UAnimCurveCompressionSettings::UAnimCurveCompressionSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Codec = ObjectInitializer.CreateDefaultSubobject<UAnimCurveCompressionCodec_CompressedRichCurve>(FName("CurveCompressionCodec"));
}

}
