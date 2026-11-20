#pragma once

#include "Engine/Plugins/Runtime/PhysXVehicles/Source/PhysXVehicles/WheeledVehicleMovementComponent.h"

#include "WheeledVehicleMovementComponent4W.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UWheeledVehicleMovementComponent4W : public UWheeledVehicleMovementComponent {
    GENERATED_BODY()
public:
    explicit UWheeledVehicleMovementComponent4W(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
