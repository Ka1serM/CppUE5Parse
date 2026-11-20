#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionPanner.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionPanner : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput Coordinate;
    UPROPERTY()
    FExpressionInput Time;
    UPROPERTY()
    FExpressionInput Speed;
    UPROPERTY()
    float SpeedX = 0.0f;
    UPROPERTY()
    float SpeedY = 0.0f;
    UPROPERTY()
    uint32 ConstCoordinate = 0;
    UPROPERTY()
    bool bFractionalPart = false;
};

}
