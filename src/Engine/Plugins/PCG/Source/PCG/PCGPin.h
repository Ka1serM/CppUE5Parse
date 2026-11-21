#pragma once

#include "Engine/Plugins/PCG/Source/PCG/Data/Registry/PCGDataTypeIdentifier.h"
#include "Engine/Plugins/PCG/Source/PCG/PCGCommon.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "PCGPin.generated.h"

namespace ue5parse::runtime {

class UPCGEdge;
class UPCGNode;

UENUM()
enum class EPCGPinUsage : uint8 { Normal, Loop, Feedback, DependencyOnly };

UENUM()
enum class EPCGPinStatus : uint8 { Normal, Required, Advanced, OverrideOrUserParam };

USTRUCT()
struct FPCGPinProperties {
    GENERATED_BODY()

    UPROPERTY()
    FName Label;
    //! An EPCGDataType enum until 5.7.
    UPROPERTY()
    FPCGDataTypeIdentifier AllowedTypes;
    UPROPERTY()
    bool bAllowMultipleConnections = true;
    UPROPERTY()
    bool bAllowMultipleData = true;
    UPROPERTY()
    bool bAdvancedPin = false;
    UPROPERTY()
    EPCGPinUsage Usage = EPCGPinUsage::Normal;
    UPROPERTY()
    EPCGPinStatus PinStatus = EPCGPinStatus::Normal;
    UPROPERTY()
    bool bInvisiblePin = false;
};

UCLASS()
class UPCGPin : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGPin(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UPCGNode> Node = nullptr;
    UPROPERTY()
    FName Label_DEPRECATED;
    UPROPERTY()
    TArray<TObjectPtr<UPCGEdge>> Edges;
    UPROPERTY()
    FPCGPinProperties Properties;
};

}
