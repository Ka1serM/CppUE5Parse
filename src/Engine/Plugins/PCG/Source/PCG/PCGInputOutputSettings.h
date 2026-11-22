#pragma once

#include "Engine/Plugins/PCG/Source/PCG/PCGPin.h"
#include "Engine/Plugins/PCG/Source/PCG/PCGSettings.h"

#include "PCGInputOutputSettings.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPCGGraphInputOutputSettings : public UPCGSettings {
    GENERATED_BODY()
public:
    explicit UPCGGraphInputOutputSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TSet<FName> PinLabels_DEPRECATED;
    UPROPERTY()
    TArray<FPCGPinProperties> Pins;
    UPROPERTY()
    TArray<FPCGPinProperties> CustomPins;
    UPROPERTY()
    bool bShowAdvancedPins = false;
    UPROPERTY()
    bool bHasAddedDefaultPin = false;
};

}
