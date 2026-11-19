#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "BodyInstanceCore.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FBodyInstanceCore {
    GENERATED_BODY()
    UPROPERTY()
    bool bSimulatePhysics = false;
    UPROPERTY()
    bool bOverrideMass = false;
    UPROPERTY()
    bool bEnableGravity = true;
    UPROPERTY()
    bool bAutoWeld = false;
    UPROPERTY()
    bool bStartAwake = true;
    UPROPERTY()
    bool bGenerateWakeEvents = false;
    UPROPERTY()
    bool bUpdateMassWhenScaleChanges = false;
    UPROPERTY()
    bool bUpdateKinematicFromSimulation = false;
    UPROPERTY()
    bool bGyroscopicTorqueEnabled = false;
};

}
