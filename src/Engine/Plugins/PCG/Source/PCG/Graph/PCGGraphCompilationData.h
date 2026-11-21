#pragma once

#include "Engine/Plugins/PCG/Source/PCG/Graph/PCGGraphTask.h"
#include "Engine/Plugins/PCG/Source/PCG/Graph/PCGStackContext.h"

#include "PCGGraphCompilationData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPCGGraphTasks {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPCGGraphTask> GraphTasks;
};

UCLASS()
class UPCGGraphCompilationData : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGGraphCompilationData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TMap<uint32, FPCGGraphTasks> Tasks;
    UPROPERTY()
    TMap<uint32, FPCGStackContext> StackContexts;
};

}
