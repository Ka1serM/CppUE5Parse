#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "PCGData.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UPCGData : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    uint64 UID = 0;
};

}
