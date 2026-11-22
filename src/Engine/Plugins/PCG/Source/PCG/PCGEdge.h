#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PCGEdge.generated.h"

namespace ue5parse::runtime {

class UPCGNode;
class UPCGPin;

UCLASS()
class UPCGEdge : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGEdge(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FName InboundLabel_DEPRECATED;
    UPROPERTY()
    TObjectPtr<UPCGNode> InboundNode_DEPRECATED = nullptr;
    UPROPERTY()
    FName OutboundLabel_DEPRECATED;
    UPROPERTY()
    TObjectPtr<UPCGNode> OutboundNode_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UPCGPin> InputPin = nullptr;
    UPROPERTY()
    TObjectPtr<UPCGPin> OutputPin = nullptr;
};

}
