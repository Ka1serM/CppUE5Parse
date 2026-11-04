#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompressionTypes.h"

#include "AnimCurveCompressionCodec.generated.h"

namespace ue5parse::runtime {

struct FBlendedCurve;

UCLASS(Abstract)
class UAnimCurveCompressionCodec : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
    virtual UAnimCurveCompressionCodec* GetCodec(const FString& Path) { return this; }
    virtual void DecompressCurves(const FCompressedAnimSequence& AnimSeq, FBlendedCurve& Curves, float CurrentTime) const {}
    virtual float DecompressCurve(const FCompressedAnimSequence& AnimSeq, FName CurveName, float CurrentTime) const { return 0.0f; }

    FGuid InstanceGuid;
};

}
