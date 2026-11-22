#pragma once

#include "Engine/Source/Runtime/Engine/Sound/SoundNode.h"

#include "SoundNodeModulatorContinuous.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ModulationParamMode : int32 {
    MPM_Normal,
    MPM_Abs,
    MPM_Direct,
    MPM_MAX,
};

USTRUCT()
struct FModulatorContinuousParams {
    GENERATED_BODY()
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    float Default = 1.f;
    UPROPERTY()
    float MinInput = 0.f;
    UPROPERTY()
    float MaxInput = 1.f;
    UPROPERTY()
    float MinOutput = 0.f;
    UPROPERTY()
    float MaxOutput = 1.f;
    UPROPERTY()
    TEnumAsByte<ModulationParamMode> ParamMode = MPM_Normal;
};

UCLASS()
class USoundNodeModulatorContinuous : public USoundNode {
    GENERATED_BODY()
public:
    UPROPERTY()
    FModulatorContinuousParams PitchModulationParams;
    UPROPERTY()
    FModulatorContinuousParams VolumeModulationParams;
};

}
