#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCompressionTypes.h"

#include "AnimCurveCompressionCodec.generated.h"

namespace ue5parse::runtime {

struct FBlendedCurve;

// UAnimCurveCompressionCodec (UE's Animation/AnimCurveCompressionCodec.h): one way to compress curves.
UCLASS(Abstract)
class UAnimCurveCompressionCodec : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
    // This codec for every path (UE's GetCodec of a codec without children).
    virtual UAnimCurveCompressionCodec* GetCodec(const FString& Path) { return this; }
    // Every curve of the sequence at a time, into Curves; one curve by name.
    virtual void DecompressCurves(const FCompressedAnimSequence& AnimSeq, FBlendedCurve& Curves, float CurrentTime) const {}
    virtual float DecompressCurve(const FCompressedAnimSequence& AnimSeq, FName CurveName, float CurrentTime) const { return 0.0f; }

    FGuid InstanceGuid;
};

}
