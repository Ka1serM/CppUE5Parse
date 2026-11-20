#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionTextureSample.h"

#include "MaterialExpressionTextureSampleParameter.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UMaterialExpressionTextureSampleParameter : public UMaterialExpressionTextureSample {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FGuid ExpressionGUID;
    UPROPERTY()
    FName Group;
};

}
