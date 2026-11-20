#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionParameter.h"

#include "MaterialExpressionStaticBoolParameter.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionStaticBoolParameter : public UMaterialExpressionParameter {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool DefaultValue = false;
};

}
