#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionParameter.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UMaterialExpressionParameter : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName ParameterName = FName("Param");
    UPROPERTY()
    FGuid ExpressionGUID;
    UPROPERTY()
    FName Group;
    UPROPERTY()
    int32 SortPriority = 0;
};

}
