#pragma once

#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"

#include "GroomAssetPhysics.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EGroomNiagaraSolvers : uint8 { None = 0, CosseratRods = 0x02, AngularSprings = 0x04, CustomSolver = 0x08 };

UENUM()
enum class EGroomStrandsSize : uint8 { None = 0, Size2 = 0x02, Size4 = 0x04, Size8 = 0x08, Size16 = 0x10, Size32 = 0x20 };

USTRUCT()
struct FHairSolverSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool EnableSimulation = false;
    UPROPERTY()
    EGroomNiagaraSolvers NiagaraSolver = EGroomNiagaraSolvers::AngularSprings;
    UPROPERTY()
    TSoftObjectPtr<UObject> CustomSystem;
    UPROPERTY()
    int32 SubSteps = 5;
    UPROPERTY()
    int32 IterationCount = 5;
    UPROPERTY()
    float GravityPreloading = 0.0f;
    UPROPERTY()
    bool bForceVisible = false;
    UPROPERTY()
    bool bEnableDeformation = false;
};

USTRUCT()
struct FHairExternalForces {
    GENERATED_BODY()
    UPROPERTY()
    FVector GravityVector = FVector(0.0, 0.0, -981.0);
    UPROPERTY()
    float AirDrag = 0.1f;
    UPROPERTY()
    FVector AirVelocity = FVector(0.0, 0.0, 0.0);
};

USTRUCT()
struct FHairBendConstraint {
    GENERATED_BODY()
    FHairBendConstraint();
    UPROPERTY()
    bool SolveBend = true;
    UPROPERTY()
    bool ProjectBend = false;
    UPROPERTY()
    float BendDamping = 0.005f;
    UPROPERTY()
    float BendStiffness = 0.01f;
    UPROPERTY()
    FRuntimeFloatCurve BendScale;
};

USTRUCT()
struct FHairStretchConstraint {
    GENERATED_BODY()
    FHairStretchConstraint();
    UPROPERTY()
    bool SolveStretch = true;
    UPROPERTY()
    bool ProjectStretch = false;
    UPROPERTY()
    float StretchDamping = 0.005f;
    UPROPERTY()
    float StretchStiffness = 1.0f;
    UPROPERTY()
    FRuntimeFloatCurve StretchScale;
};

USTRUCT()
struct FHairCollisionConstraint {
    GENERATED_BODY()
    FHairCollisionConstraint();
    UPROPERTY()
    bool SolveCollision = true;
    UPROPERTY()
    bool ProjectCollision = true;
    UPROPERTY()
    float StaticFriction = 0.1f;
    UPROPERTY()
    float KineticFriction = 0.1f;
    UPROPERTY()
    float StrandsViscosity = 1.0f;
    UPROPERTY()
    FIntVector GridDimension = FIntVector(30, 30, 30);
    UPROPERTY()
    float CollisionRadius = 0.1f;
    UPROPERTY()
    FRuntimeFloatCurve RadiusScale;
};

USTRUCT()
struct FHairMaterialConstraints {
    GENERATED_BODY()
    UPROPERTY()
    FHairBendConstraint BendConstraint;
    UPROPERTY()
    FHairStretchConstraint StretchConstraint;
    UPROPERTY()
    FHairCollisionConstraint CollisionConstraint;
};

USTRUCT()
struct FHairStrandsParameters {
    GENERATED_BODY()
    FHairStrandsParameters();
    UPROPERTY()
    EGroomStrandsSize StrandsSize = EGroomStrandsSize::Size8;
    UPROPERTY()
    float StrandsDensity = 1.0f;
    UPROPERTY()
    float StrandsSmoothing = 0.1f;
    UPROPERTY()
    float StrandsThickness = 0.01f;
    UPROPERTY()
    FRuntimeFloatCurve ThicknessScale;
};

USTRUCT()
struct FHairGroupsPhysics {
    GENERATED_BODY()
    UPROPERTY()
    FHairSolverSettings SolverSettings;
    UPROPERTY()
    FHairExternalForces ExternalForces;
    UPROPERTY()
    FHairMaterialConstraints MaterialConstraints;
    UPROPERTY()
    FHairStrandsParameters StrandsParameters;
};

}
