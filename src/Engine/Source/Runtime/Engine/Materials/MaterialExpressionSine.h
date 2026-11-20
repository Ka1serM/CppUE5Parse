#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionSine.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionSine : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput Input;
    UPROPERTY()
    float Period = 1.0f;
};

}
