#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionParameter.h"

#include "MaterialExpressionScalarParameter.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionScalarParameter : public UMaterialExpressionParameter {
    GENERATED_BODY()
public:
    UPROPERTY()
    float DefaultValue = 0.0f;
    UPROPERTY()
    bool bUseCustomPrimitiveData = false;
    UPROPERTY()
    uint8 PrimitiveDataIndex = 0;
};

}
