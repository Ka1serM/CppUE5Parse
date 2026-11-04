#include "Engine/Source/Runtime/Engine/Animation/AnimCurveCompressionSettings.h"

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveCompressionCodec_CompressedRichCurve.h"

namespace ue5parse::runtime {

UAnimCurveCompressionSettings::UAnimCurveCompressionSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Codec = ObjectInitializer.CreateDefaultSubobject<UAnimCurveCompressionCodec_CompressedRichCurve>(FName("CurveCompressionCodec"));
}

}
