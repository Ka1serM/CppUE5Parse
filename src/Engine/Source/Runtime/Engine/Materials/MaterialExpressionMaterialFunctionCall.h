#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionMaterialFunctionCall.generated.h"

namespace ue5parse::runtime {

class UMaterialFunctionInterface;

USTRUCT()
struct FFunctionExpressionInput {
    GENERATED_BODY()
    UPROPERTY()
    FGuid ExpressionInputId;
    UPROPERTY()
    FExpressionInput Input;
};

USTRUCT()
struct FFunctionExpressionOutput {
    GENERATED_BODY()
    UPROPERTY()
    FGuid ExpressionOutputId;
    UPROPERTY()
    FExpressionOutput Output;
};

UCLASS()
class UMaterialExpressionMaterialFunctionCall : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UMaterialFunctionInterface> MaterialFunction = nullptr;
    UPROPERTY()
    TArray<FFunctionExpressionInput> FunctionInputs;
    UPROPERTY()
    TArray<FFunctionExpressionOutput> FunctionOutputs;
};

}
