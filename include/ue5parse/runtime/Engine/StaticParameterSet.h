#pragma once

#include "ue5parse/runtime/Engine/MaterialTypes.h"

#include "StaticParameterSet.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FStaticParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    FMaterialParameterInfo ParameterInfo;
    UPROPERTY()
    bool bOverride = false;
    UPROPERTY()
    FGuid ExpressionGUID;
};

USTRUCT()
struct FStaticSwitchParameter : public FStaticParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    bool Value = false;
};

}
