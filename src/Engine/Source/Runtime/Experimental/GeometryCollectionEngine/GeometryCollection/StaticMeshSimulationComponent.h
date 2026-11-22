#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Experimental/ChaosSolverEngine/Chaos/ChaosNotifyHandlerInterface.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionSimulationTypes.h"

#include "StaticMeshSimulationComponent.generated.h"

namespace ue5parse::runtime {

class AChaosSolverActor;
class UChaosPhysicalMaterial;
class UPrimitiveComponent;

UCLASS()
class UStaticMeshSimulationComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UStaticMeshSimulationComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool Simulating = true;
    UPROPERTY()
    bool bNotifyCollisions = false;
    UPROPERTY()
    EObjectStateTypeEnum ObjectType = EObjectStateTypeEnum::Chaos_Object_Dynamic;
    UPROPERTY()
    float Mass = 1.0f;
    UPROPERTY()
    ECollisionTypeEnum CollisionType = ECollisionTypeEnum::Chaos_Surface_Volumetric;
    UPROPERTY()
    EImplicitTypeEnum ImplicitType = EImplicitTypeEnum::Chaos_Max;
    UPROPERTY()
    int32 MinLevelSetResolution = 5;
    UPROPERTY()
    int32 MaxLevelSetResolution = 10;
    UPROPERTY()
    EInitialVelocityTypeEnum InitialVelocityType = EInitialVelocityTypeEnum::Chaos_Initial_Velocity_User_Defined;
    UPROPERTY()
    FVector InitialLinearVelocity{0.0, 0.0, 0.0};
    UPROPERTY()
    FVector InitialAngularVelocity{0.0, 0.0, 0.0};
    UPROPERTY()
    float DamageThreshold = 0.0f;
    UPROPERTY()
    float Friction = 0.0f;
    UPROPERTY()
    float Bouncyness = 0.0f;
    UPROPERTY()
    TObjectPtr<UChaosPhysicalMaterial> PhysicalMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<AChaosSolverActor> ChaosSolverActor = nullptr;
    UPROPERTY()
    FOnChaosPhysicsCollision OnChaosPhysicsCollision;
    UPROPERTY()
    TArray<TObjectPtr<UPrimitiveComponent>> SimulatedComponents;
};

}
