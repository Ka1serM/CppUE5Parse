#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionFunctionOutput.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionFunctionOutput : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName OutputName = FName("Result");
    UPROPERTY()
    FString Description;
    UPROPERTY()
    int32 SortPriority = 0;
    UPROPERTY()
    FExpressionInput A;
    UPROPERTY()
    bool bLastPreviewed = false;
    UPROPERTY()
    FGuid Id;
};

}
