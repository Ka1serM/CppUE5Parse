#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "PCGPinDependencyExpression.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPCGPinDependencyExpression {
    GENERATED_BODY()

    UPROPERTY()
    TArray<uint64> Expression;
};

}
