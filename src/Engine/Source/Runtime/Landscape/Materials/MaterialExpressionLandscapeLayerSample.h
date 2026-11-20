#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionLandscapeLayerSample.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionLandscapeLayerSample : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    float PreviewWeight = 0.0f;
    UPROPERTY()
    FGuid ExpressionGUID;
};

}
