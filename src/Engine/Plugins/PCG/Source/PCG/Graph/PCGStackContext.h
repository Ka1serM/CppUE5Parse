#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "PCGStackContext.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPCGStackFrame {
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<UObject> Object;
    UPROPERTY()
    int32 LoopIndex = INDEX_NONE;
};

USTRUCT()
struct FPCGStack {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPCGStackFrame> StackFrames;
};

USTRUCT()
struct FPCGStackContext {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPCGStack> Stacks;
    UPROPERTY()
    int32 CurrentStackIndex = INDEX_NONE;
};

}
