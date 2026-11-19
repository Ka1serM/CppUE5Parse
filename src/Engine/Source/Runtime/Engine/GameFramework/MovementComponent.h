#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MovementComponent.generated.h"

namespace ue5parse::runtime {

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

    UPROPERTY(Transient, DuplicateTransient)
    TObjectPtr<USceneComponent> UpdatedComponent = nullptr;
    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;
    UPROPERTY(EditDefaultsOnly)
    bool bAutoRegisterUpdatedComponent = true;

private:
    void AutoRegisterUpdatedComponent();
};

}
