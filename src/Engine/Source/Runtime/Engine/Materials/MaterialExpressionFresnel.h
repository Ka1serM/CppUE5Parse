#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionFresnel.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionFresnel : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput ExponentIn;
    UPROPERTY()
    float Exponent = 5.0f;
    UPROPERTY()
    FExpressionInput BaseReflectFractionIn;
    UPROPERTY()
    float BaseReflectFraction = 0.04f;
    UPROPERTY()
    FExpressionInput Normal;
};

}
