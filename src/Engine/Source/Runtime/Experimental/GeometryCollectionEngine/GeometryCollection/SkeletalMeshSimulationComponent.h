#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Experimental/ChaosSolverEngine/Chaos/ChaosNotifyHandlerInterface.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionSimulationTypes.h"

#include "SkeletalMeshSimulationComponent.generated.h"

namespace ue5parse::runtime {

class AChaosSolverActor;
class UChaosPhysicalMaterial;
class UPhysicsAsset;

UCLASS()
class USkeletalMeshSimulationComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit USkeletalMeshSimulationComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UChaosPhysicalMaterial> PhysicalMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<AChaosSolverActor> ChaosSolverActor = nullptr;
    UPROPERTY()
    TObjectPtr<UPhysicsAsset> OverridePhysicsAsset = nullptr;
    UPROPERTY()
    bool bSimulating = true;
    UPROPERTY()
    bool bNotifyCollisions = false;
    UPROPERTY()
    EObjectStateTypeEnum ObjectType = EObjectStateTypeEnum::Chaos_Object_Kinematic;
    UPROPERTY()
    float Density = 2.4f;
    UPROPERTY()
    float MinMass = 0.001f;
    UPROPERTY()
    float MaxMass = 1.e6f;
    UPROPERTY()
    ECollisionTypeEnum CollisionType = ECollisionTypeEnum::Chaos_Volumetric;
    UPROPERTY()
    float ImplicitShapeParticlesPerUnitArea = 0.1f;
    UPROPERTY()
    int32 ImplicitShapeMinNumParticles = 0;
    UPROPERTY()
    int32 ImplicitShapeMaxNumParticles = 50;
    UPROPERTY()
    int32 MinLevelSetResolution = 5;
    UPROPERTY()
    int32 MaxLevelSetResolution = 10;
    UPROPERTY()
    int32 CollisionGroup = 0;
    UPROPERTY()
    bool bEnableClustering = false;
    UPROPERTY()
    int32 ClusterGroupIndex = 0;
    UPROPERTY()
    int32 MaxClusterLevel = 100;
    UPROPERTY()
    float DamageThreshold = 250.0f;
    UPROPERTY()
    EInitialVelocityTypeEnum InitialVelocityType = EInitialVelocityTypeEnum::Chaos_Initial_Velocity_User_Defined;
    UPROPERTY()
    FVector InitialLinearVelocity{0.0, 0.0, 0.0};
    UPROPERTY()
    FVector InitialAngularVelocity{0.0, 0.0, 0.0};
    UPROPERTY()
    FOnChaosPhysicsCollision OnChaosPhysicsCollision;
};

}
