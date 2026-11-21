#pragma once

#include "Engine/Plugins/PCG/Source/PCG/Graph/PCGPinDependencyExpression.h"
#include "Engine/Plugins/PCG/Source/PCG/PCGPin.h"
#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "PCGGraphTask.generated.h"

namespace ue5parse::runtime {

class UPCGSettings;

inline constexpr uint64 InvalidPCGTaskId = static_cast<uint64>(-1);

UENUM()
enum class EPCGElementSource : int32 { Trivial, TrivialPostGraph, Gather, FromNode, FromCookedSettings };

USTRUCT()
struct FPCGGraphTaskInput {
    GENERATED_BODY()

    UPROPERTY()
    uint64 TaskId = InvalidPCGTaskId;
    UPROPERTY()
    TOptional<FPCGPinProperties> UpstreamPin;
    UPROPERTY()
    TOptional<FPCGPinProperties> DownstreamPin;
    UPROPERTY()
    bool bProvideData = false;
    UPROPERTY()
    bool bIsUsedMultipleTimes = true;
};

USTRUCT()
struct FPCGGraphTask {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPCGGraphTaskInput> Inputs;
    UPROPERTY()
    EPCGElementSource ElementSource = EPCGElementSource::FromNode;
    UPROPERTY()
    TSoftObjectPtr<UPCGNode> NodePtr;
    UPROPERTY()
    TObjectPtr<const UPCGSettings> CookedSettings = nullptr;
    UPROPERTY()
    uint64 NodeId = InvalidPCGTaskId;
    UPROPERTY()
    uint64 CompiledTaskId = InvalidPCGTaskId;
    UPROPERTY()
    uint64 ParentId = InvalidPCGTaskId;
    UPROPERTY()
    FPCGPinDependencyExpression PinDependency;
    UPROPERTY()
    int32 StackIndex = INDEX_NONE;
};

}
