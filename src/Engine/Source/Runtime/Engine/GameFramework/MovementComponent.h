#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MovementComponent.generated.h"

namespace ue5parse::runtime {

class UPrimitiveComponent;

UENUM()
enum class EPlaneConstraintAxisSetting : uint8 { Custom, X, Y, Z, UseGlobalPhysicsSetting };

UCLASS(Abstract)
class UMovementComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { bWantsInitializeComponent = true; }

    void OnRegister() override;
    void InitializeComponent() override;
    virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent);
    virtual bool ShouldSkipUpdate(float DeltaTime) const;
    virtual void UpdateComponentVelocity();
    virtual bool CheckStillInWorld();

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> UpdatedComponent = nullptr;
    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;
    UPROPERTY()
    FVector PlaneConstraintNormal = FVector::ZeroVector;
    UPROPERTY()
    FVector PlaneConstraintOrigin;
    UPROPERTY()
    uint8 bConstrainToPlane : 1 = false;
    UPROPERTY()
    uint8 bSnapToPlaneAtStart : 1 = false;
    UPROPERTY()
    uint8 bUpdateOnlyIfRendered : 1 = false;
    UPROPERTY()
    uint8 bAutoUpdateTickRegistration : 1 = true;
    UPROPERTY()
    uint8 bAutoRegisterUpdatedComponent : 1 = true;
    UPROPERTY()
    EPlaneConstraintAxisSetting PlaneConstraintAxisSetting = EPlaneConstraintAxisSetting::Custom;
    UPROPERTY(Transient)
    TObjectPtr<UPrimitiveComponent> UpdatedPrimitive = nullptr;
    UPROPERTY()
    uint8 bTickBeforeOwner : 1 = true;
    UPROPERTY()
    uint8 bAutoRegisterPhysicsVolumeUpdates : 1 = true;
    UPROPERTY()
    uint8 bComponentShouldUpdatePhysicsVolume : 1 = true;

private:
    void AutoRegisterUpdatedComponent();
};

}
