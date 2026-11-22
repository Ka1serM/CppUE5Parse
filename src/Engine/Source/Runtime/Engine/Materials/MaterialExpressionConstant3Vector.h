#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"

#include "MaterialExpressionConstant3Vector.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionConstant3Vector : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FLinearColor Constant;
};

}
