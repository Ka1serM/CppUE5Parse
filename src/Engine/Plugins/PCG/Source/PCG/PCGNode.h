#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PCGNode.generated.h"

namespace ue5parse::runtime {

class UPCGEdge;
class UPCGPin;
class UPCGSettings;
class UPCGSettingsInterface;

UCLASS()
class UPCGNode : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGNode(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UPCGSettings> DefaultSettings = nullptr;
    UPROPERTY()
    TObjectPtr<UPCGSettingsInterface> SettingsInterface = nullptr;
    UPROPERTY()
    FName NodeTitle;
    UPROPERTY()
    TArray<TObjectPtr<UPCGNode>> OutboundNodes_DEPRECATED;
    UPROPERTY()
    TArray<TObjectPtr<UPCGEdge>> InboundEdges_DEPRECATED;
    UPROPERTY()
    TArray<TObjectPtr<UPCGEdge>> OutboundEdges_DEPRECATED;
    UPROPERTY()
    TArray<TObjectPtr<UPCGPin>> InputPins;
    UPROPERTY()
    TArray<TObjectPtr<UPCGPin>> OutputPins;
};

}
