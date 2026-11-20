#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionMultiply.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionMultiply : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput A;
    UPROPERTY()
    FExpressionInput B;
    UPROPERTY()
    float ConstA = 0.0f;
    UPROPERTY()
    float ConstB = 1.0f;
};

}
