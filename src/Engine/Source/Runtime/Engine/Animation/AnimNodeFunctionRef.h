#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "AnimNodeFunctionRef.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimNodeFunctionRef {
    GENERATED_BODY()
    UPROPERTY()
    FName ClassName;
    UPROPERTY()
    FName FunctionName;
    UPROPERTY(Transient)
    TObjectPtr<const UClass> Class = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UFunction> Function = nullptr;
};

}
