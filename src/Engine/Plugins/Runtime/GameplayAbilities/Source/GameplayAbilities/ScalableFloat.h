#pragma once

#include "Engine/Plugins/Runtime/DataRegistry/Source/DataRegistry/DataRegistryId.h"
#include "Engine/Source/Runtime/Engine/Engine/CurveTable.h"

#include "ScalableFloat.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FScalableFloat {
    GENERATED_BODY()
public:
    FScalableFloat() = default;
    explicit FScalableFloat(float InitialValue) : Value(InitialValue) {}

    UPROPERTY()
    float Value = 0;
    UPROPERTY()
    FCurveTableRowHandle Curve;
    UPROPERTY()
    FDataRegistryType RegistryType;
};

}
