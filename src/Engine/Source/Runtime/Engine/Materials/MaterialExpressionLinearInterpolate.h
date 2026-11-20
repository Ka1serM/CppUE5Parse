#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionLinearInterpolate.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionLinearInterpolate : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput A;
    UPROPERTY()
    FExpressionInput B;
    UPROPERTY()
    FExpressionInput Alpha;
    UPROPERTY()
    float ConstA = 0.0f;
    UPROPERTY()
    float ConstB = 1.0f;
    UPROPERTY()
    float ConstAlpha = 0.5f;
};

}
