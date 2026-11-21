#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PCGGraph.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UPCGGraphInterface : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGGraphInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UPCGGraphInstance : public UPCGGraphInterface {
    GENERATED_BODY()
public:
    explicit UPCGGraphInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UPCGGraphInterface> Graph = nullptr;
};

}
