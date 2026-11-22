#pragma once

#include "Engine/Plugins/Runtime/PhysXVehicles/Source/PhysXVehicles/WheeledVehicleMovementComponent.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"

#include "WheeledVehicleMovementComponent4W.generated.h"

namespace ue5parse::runtime {

namespace EVehicleDifferential4W {

UENUM()
enum Type : int32 { LimitedSlip_4W, LimitedSlip_FrontDrive, LimitedSlip_RearDrive, Open_4W, Open_FrontDrive, Open_RearDrive };

}

USTRUCT()
struct FVehicleDifferential4WData {
    GENERATED_BODY()

    UPROPERTY()
    TEnumAsByte<EVehicleDifferential4W::Type> DifferentialType = EVehicleDifferential4W::LimitedSlip_4W;
    UPROPERTY()
    float FrontRearSplit = 0.0f;
    UPROPERTY()
    float FrontLeftRightSplit = 0.0f;
    UPROPERTY()
    float RearLeftRightSplit = 0.0f;
    UPROPERTY()
    float CentreBias = 0.0f;
    UPROPERTY()
    float FrontBias = 0.0f;
    UPROPERTY()
    float RearBias = 0.0f;
};

USTRUCT()
struct FVehicleEngineData {
    GENERATED_BODY()

    UPROPERTY()
    FRuntimeFloatCurve TorqueCurve;
    UPROPERTY()
    float MaxRPM = 0.0f;
    UPROPERTY()
    float MOI = 0.0f;
    UPROPERTY()
    float DampingRateFullThrottle = 0.0f;
    UPROPERTY()
    float DampingRateZeroThrottleClutchEngaged = 0.0f;
    UPROPERTY()
    float DampingRateZeroThrottleClutchDisengaged = 0.0f;
};

USTRUCT()
struct FVehicleGearData {
    GENERATED_BODY()

    UPROPERTY()
    float Ratio = 0.0f;
    UPROPERTY()
    float DownRatio = 0.0f;
    UPROPERTY()
    float UpRatio = 0.0f;
};

USTRUCT()
struct FVehicleTransmissionData {
    GENERATED_BODY()

    UPROPERTY()
    bool bUseGearAutoBox = false;
    UPROPERTY()
    float GearSwitchTime = 0.0f;
    UPROPERTY()
    float GearAutoBoxLatency = 0.0f;
    UPROPERTY()
    float FinalRatio = 0.0f;
    UPROPERTY()
    TArray<FVehicleGearData> ForwardGears;
    UPROPERTY()
    float ReverseGearRatio = 0.0f;
    UPROPERTY()
    float NeutralGearUpRatio = 0.0f;
    UPROPERTY()
    float ClutchStrength = 0.0f;
};

UCLASS()
class UWheeledVehicleMovementComponent4W : public UWheeledVehicleMovementComponent {
    GENERATED_BODY()
public:
    explicit UWheeledVehicleMovementComponent4W(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FVehicleEngineData EngineSetup;
    UPROPERTY()
    FVehicleDifferential4WData DifferentialSetup;
    UPROPERTY()
    float AckermannAccuracy = 0.0f;
    UPROPERTY()
    FVehicleTransmissionData TransmissionSetup;
    UPROPERTY()
    FRuntimeFloatCurve SteeringCurve;
};

}
