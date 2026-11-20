#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionTextureCoordinate.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionTextureCoordinate : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 CoordinateIndex = 0;
    UPROPERTY()
    float UTiling = 1.0f;
    UPROPERTY()
    float VTiling = 1.0f;
    UPROPERTY()
    bool UnMirrorU = false;
    UPROPERTY()
    bool UnMirrorV = false;
};

}
