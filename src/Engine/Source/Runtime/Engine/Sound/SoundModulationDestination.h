#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "SoundModulationDestination.generated.h"

namespace ue5parse::runtime {

class USoundModulatorBase;

UENUM()
enum class EModulationRouting : uint8 {
    Disable,
    Inherit,
    Override,
    Union,
};

USTRUCT()
struct FSoundModulationDestinationSettings {
    GENERATED_BODY()
    UPROPERTY()
    float Value = 1.0f;
    UPROPERTY()
    bool bEnableModulation = false;
    UPROPERTY()
    TObjectPtr<USoundModulatorBase> Modulator = nullptr;
    UPROPERTY()
    TSet<TObjectPtr<USoundModulatorBase>> Modulators;
};

USTRUCT()
struct FSoundModulationDefaultSettings {
    GENERATED_BODY()
    UPROPERTY()
    FSoundModulationDestinationSettings VolumeModulationDestination;
    UPROPERTY()
    FSoundModulationDestinationSettings PitchModulationDestination;
    UPROPERTY()
    FSoundModulationDestinationSettings HighpassModulationDestination;
    UPROPERTY()
    FSoundModulationDestinationSettings LowpassModulationDestination;
};

USTRUCT()
struct FSoundModulationDefaultRoutingSettings : public FSoundModulationDefaultSettings {
    GENERATED_BODY()
    UPROPERTY()
    EModulationRouting VolumeRouting = EModulationRouting::Inherit;
    UPROPERTY()
    EModulationRouting PitchRouting = EModulationRouting::Inherit;
    UPROPERTY()
    EModulationRouting HighpassRouting = EModulationRouting::Inherit;
    UPROPERTY()
    EModulationRouting LowpassRouting = EModulationRouting::Inherit;
};

}
