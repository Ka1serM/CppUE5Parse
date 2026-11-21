#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

#include "NiagaraStatelessDistribution.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENiagaraDistributionMode : uint8 { Binding, UniformConstant, NonUniformConstant, UniformRange, NonUniformRange, UniformCurve, NonUniformCurve, Expression, ColorGradient, Array };

USTRUCT()
struct FNiagaraDistributionBase {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraDistributionMode Mode = ENiagaraDistributionMode::UniformConstant;
    UPROPERTY()
    FNiagaraVariableBase ParameterBinding;
    UPROPERTY()
    FInstancedStruct ParameterExpression;
    UPROPERTY()
    uint8 InterpolationMode : 1 = 0;
    UPROPERTY()
    uint8 AddressMode : 1 = 0;
    UPROPERTY()
    uint8 LookupValueMode = 0;
};

USTRUCT()
struct FNiagaraDistributionRangeFloat : public FNiagaraDistributionBase {
    GENERATED_BODY()

    UPROPERTY()
    float Min = 0;
    UPROPERTY()
    float Max = 0;

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FNiagaraDistributionRangeFloat> : TStructOpsTypeTraitsBase2<FNiagaraDistributionRangeFloat> {
    enum { WithSerializeFromMismatchedTag = true };
};

USTRUCT()
struct FNiagaraDistributionRangeInt {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraDistributionMode Mode = ENiagaraDistributionMode::UniformConstant;
    UPROPERTY()
    FNiagaraVariableBase ParameterBinding;
    UPROPERTY()
    int32 Min = 0;
    UPROPERTY()
    int32 Max = 0;
    UPROPERTY()
    FInstancedStruct ParameterExpression;
};

}
