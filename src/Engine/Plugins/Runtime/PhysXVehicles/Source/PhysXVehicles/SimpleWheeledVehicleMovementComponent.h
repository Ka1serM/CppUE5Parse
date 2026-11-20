#pragma once

#include "Engine/Plugins/Runtime/PhysXVehicles/Source/PhysXVehicles/WheeledVehicleMovementComponent.h"

#include "SimpleWheeledVehicleMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USimpleWheeledVehicleMovementComponent : public UWheeledVehicleMovementComponent {
    GENERATED_BODY()
public:
    explicit USimpleWheeledVehicleMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
