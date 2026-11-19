#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/PhysicsCore/Chaos/ChaosEngineInterface.h"

#include "PhysicalMaterial.generated.h"

namespace ue5parse::runtime {

class UTireType;

USTRUCT()
struct FTireFrictionScalePair {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UTireType> TireType = nullptr;
    UPROPERTY()
    float FrictionScale = 1.0f;
};

USTRUCT()
struct FPhysicalMaterialStrength {
    GENERATED_BODY()
    UPROPERTY()
    float TensileStrength = 2.0f;
    UPROPERTY()
    float CompressionStrength = 20.0f;
    UPROPERTY()
    float ShearStrength = 6.0f;
};

USTRUCT()
struct FPhysicalMaterialDamageModifier {
    GENERATED_BODY()
    UPROPERTY()
    float DamageThresholdMultiplier = 1.0f;
};

UENUM()
enum class EPhysicalMaterialSoftCollisionMode : uint8 { None, RelativeThickness, AbsoluteThickess };

UCLASS()
class UPhysicalMaterial : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Friction = 0.7f;
    UPROPERTY()
    float StaticFriction = 0.0f;
    UPROPERTY()
    TEnumAsByte<EFrictionCombineMode::Type> FrictionCombineMode = EFrictionCombineMode::Average;
    UPROPERTY()
    bool bOverrideFrictionCombineMode = false;
    UPROPERTY()
    float Restitution = 0.3f;
    UPROPERTY()
    TEnumAsByte<EFrictionCombineMode::Type> RestitutionCombineMode = EFrictionCombineMode::Average;
    UPROPERTY()
    bool bOverrideRestitutionCombineMode = false;
    UPROPERTY()
    float Density = 1.0f;
    UPROPERTY()
    float SleepLinearVelocityThreshold = 1.0f;
    UPROPERTY()
    float SleepAngularVelocityThreshold = 0.05f;
    UPROPERTY()
    int32 SleepCounterThreshold = 4;
    UPROPERTY()
    float RaiseMassToPower = 0.75f;
    UPROPERTY()
    float DestructibleDamageThresholdScale = 1.0f;
    UPROPERTY()
    TObjectPtr<UObject> PhysicalMaterialProperty = nullptr;
    UPROPERTY()
    TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;
    UPROPERTY()
    TEnumAsByte<EPhysicalSurface> AudioSurfaceType = SurfaceType_Default;
    UPROPERTY()
    float TireFrictionScale = 1.0f;
    UPROPERTY()
    TArray<FTireFrictionScalePair> TireFrictionScales;
    UPROPERTY()
    FPhysicalMaterialStrength Strength;
    UPROPERTY()
    FPhysicalMaterialDamageModifier DamageModifier;
    UPROPERTY()
    FLinearColor DebugColor = FLinearColor(1, 1, 1, 1);
    UPROPERTY()
    bool bShowExperimentalProperties = false;
    UPROPERTY()
    EPhysicalMaterialSoftCollisionMode SoftCollisionMode = EPhysicalMaterialSoftCollisionMode::None;
    UPROPERTY()
    float SoftCollisionThickness = 0.0f;
    UPROPERTY()
    float BaseFrictionImpulse = 0.0f;
};

}
