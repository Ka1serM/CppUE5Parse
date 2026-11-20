#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialLayersFunctions.h"

#include "MaterialExpressionMaterialAttributeLayers.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionMaterialAttributeLayers : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FGuid ExpressionGUID;
    UPROPERTY()
    FMaterialAttributesInput Input;
    UPROPERTY()
    FMaterialLayersFunctions DefaultLayers;
};

}
