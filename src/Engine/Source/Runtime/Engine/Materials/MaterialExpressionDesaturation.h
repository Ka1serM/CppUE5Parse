#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"

#include "MaterialExpressionDesaturation.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionDesaturation : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput Input;
    UPROPERTY()
    FExpressionInput Fraction;
    UPROPERTY()
    FLinearColor LuminanceFactors = FLinearColor(0.3f, 0.59f, 0.11f, 0.0f);
};

}
