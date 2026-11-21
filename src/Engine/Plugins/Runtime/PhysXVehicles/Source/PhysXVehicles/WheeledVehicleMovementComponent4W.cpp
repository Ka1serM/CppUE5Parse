#include "Engine/Plugins/Runtime/PhysXVehicles/Source/PhysXVehicles/WheeledVehicleMovementComponent4W.h"

namespace ue5parse::runtime {

//! The engine, differential and transmission defaults come from the PhysX vehicle SDK data structs.
UWheeledVehicleMovementComponent4W::UWheeledVehicleMovementComponent4W(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DifferentialSetup.FrontRearSplit = 0.45f;
    DifferentialSetup.FrontLeftRightSplit = 0.5f;
    DifferentialSetup.RearLeftRightSplit = 0.5f;
    DifferentialSetup.CentreBias = 1.3f;
    DifferentialSetup.FrontBias = 1.3f;
    DifferentialSetup.RearBias = 1.3f;
    EngineSetup.MOI = 1.0f;
    EngineSetup.MaxRPM = 600.0f * 60.0f / (2.0f * 3.14159265f);
    EngineSetup.DampingRateFullThrottle = 0.15f;
    EngineSetup.DampingRateZeroThrottleClutchEngaged = 2.0f;
    EngineSetup.DampingRateZeroThrottleClutchDisengaged = 0.35f;
    FRichCurve* TorqueCurve = EngineSetup.TorqueCurve.GetRichCurve();
    TorqueCurve->AddKey(0.0f, 0.8f * 500.0f);
    TorqueCurve->AddKey(0.33f * EngineSetup.MaxRPM, 1.0f * 500.0f);
    TorqueCurve->AddKey(1.0f * EngineSetup.MaxRPM, 0.8f * 500.0f);
    TransmissionSetup.ClutchStrength = 10.0f;
    AckermannAccuracy = 1.0f;
    TransmissionSetup.GearSwitchTime = 0.5f;
    TransmissionSetup.ReverseGearRatio = -4.0f;
    TransmissionSetup.FinalRatio = 4.0f;
    TransmissionSetup.NeutralGearUpRatio = 0.65f;
    TransmissionSetup.GearAutoBoxLatency = 2.0f;
    TransmissionSetup.bUseGearAutoBox = true;
    for (const float Ratio : {4.0f, 2.0f, 1.5f, 1.1f, 1.0f}) {
        FVehicleGearData Gear;
        Gear.Ratio = Ratio;
        Gear.DownRatio = 0.5f;
        Gear.UpRatio = 0.65f;
        TransmissionSetup.ForwardGears.Add(Gear);
    }
    FRichCurve* Steering = SteeringCurve.GetRichCurve();
    Steering->AddKey(0.0f, 1.0f);
    Steering->AddKey(20.0f, 0.9f);
    Steering->AddKey(60.0f, 0.8f);
    Steering->AddKey(120.0f, 0.7f);
}

}
