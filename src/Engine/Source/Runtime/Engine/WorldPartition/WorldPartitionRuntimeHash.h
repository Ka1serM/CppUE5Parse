#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "WorldPartitionRuntimeHash.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UWorldPartitionRuntimeHash : public UObject {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeHash(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
