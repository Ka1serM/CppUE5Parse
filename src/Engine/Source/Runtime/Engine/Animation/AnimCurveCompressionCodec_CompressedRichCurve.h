#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveCompressionCodec.h"

#include "AnimCurveCompressionCodec_CompressedRichCurve.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCurveCompressionCodec_CompressedRichCurve : public UAnimCurveCompressionCodec {
    GENERATED_BODY()
public:
    void DecompressCurves(const FCompressedAnimSequence& AnimSeq, FBlendedCurve& Curves, float CurrentTime) const override;
    float DecompressCurve(const FCompressedAnimSequence& AnimSeq, FName CurveName, float CurrentTime) const override;
};

}
