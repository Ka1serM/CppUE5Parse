#pragma once

#include "ue5parse/runtime/Engine/Curves/CurveBase.h"

#include "CurveVector.generated.h"

namespace ue5parse::runtime {

class UCurveVector;

USTRUCT()
struct FRuntimeVectorCurve {
    GENERATED_BODY()
    UPROPERTY()
    FRichCurve VectorCurves[3];
    UPROPERTY()
    TObjectPtr<UCurveVector> ExternalCurve = nullptr;

    FVector GetValue(float InTime) const;
    const FRichCurve* GetRichCurveConst(int32 Index) const;
};

UCLASS()
class UCurveVector : public UCurveBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FRichCurve FloatCurves[3];

    UFUNCTION(BlueprintCallable)
    FVector GetVectorValue(float InTime) const;
    int32 GetNumCurves() const override { return 3; }
    const FRichCurve* GetCurve(int32 Index) const override { return Index >= 0 && Index < 3 ? &FloatCurves[Index] : nullptr; }
};

}
