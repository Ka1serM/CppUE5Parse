#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/NavMovementComponent.h"

#include "PawnMovementComponent.generated.h"

namespace ue5parse::runtime {

class APawn;

UCLASS(Abstract)
class UPawnMovementComponent : public UNavMovementComponent {
    GENERATED_BODY()
public:
    explicit UPawnMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY(Transient, DuplicateTransient)
    TObjectPtr<APawn> PawnOwner = nullptr;

    UFUNCTION(BlueprintCallable)
    APawn* GetPawnOwner() const { return PawnOwner; }

    void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
};

}
