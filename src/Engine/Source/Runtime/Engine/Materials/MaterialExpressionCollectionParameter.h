#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionCollectionParameter.generated.h"

namespace ue5parse::runtime {

class UMaterialParameterCollection;

UCLASS()
class UMaterialExpressionCollectionParameter : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> Collection = nullptr;
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FGuid ParameterId;
};

}
