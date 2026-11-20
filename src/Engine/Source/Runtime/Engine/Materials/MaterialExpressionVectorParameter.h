#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionParameter.h"

#include "MaterialExpressionVectorParameter.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionVectorParameter : public UMaterialExpressionParameter {
    GENERATED_BODY()
public:
    UPROPERTY()
    FLinearColor DefaultValue;
    UPROPERTY()
    bool bUseCustomPrimitiveData = false;
    UPROPERTY()
    uint8 PrimitiveDataIndex = 0;
};

}
