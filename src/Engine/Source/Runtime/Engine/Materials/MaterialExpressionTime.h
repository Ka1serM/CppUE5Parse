#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionTime.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionTime : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bIgnorePause = false;
    UPROPERTY()
    bool bOverride_Period = false;
    UPROPERTY()
    float Period = 0.0f;
};

}
