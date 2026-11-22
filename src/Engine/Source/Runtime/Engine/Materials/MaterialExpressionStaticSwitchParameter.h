#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionStaticBoolParameter.h"

#include "MaterialExpressionStaticSwitchParameter.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialExpressionStaticSwitchParameter : public UMaterialExpressionStaticBoolParameter {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput A;
    UPROPERTY()
    FExpressionInput B;
};

}
