#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NavigationObjectBase.generated.h"

namespace ue5parse::runtime {

class UCapsuleComponent;

UCLASS(Abstract)
class ANavigationObjectBase : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UCapsuleComponent> CapsuleComponent = nullptr;
    UPROPERTY()
    uint8 bIsPIEPlayerStart : 1 = false;

    explicit ANavigationObjectBase(const FObjectInitializer& ObjectInitializer);

    UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
};

}
