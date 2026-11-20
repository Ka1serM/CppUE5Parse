#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/PawnMovementComponent.h"

#include "WheeledVehicleMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UWheeledVehicleMovementComponent : public UPawnMovementComponent {
    GENERATED_BODY()
public:
    explicit UWheeledVehicleMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
