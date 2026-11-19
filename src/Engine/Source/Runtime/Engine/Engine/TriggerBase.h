#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "TriggerBase.generated.h"

namespace ue5parse::runtime {

class UShapeComponent;

UCLASS(Abstract)
class ATriggerBase : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UShapeComponent> CollisionComponent = nullptr;

    explicit ATriggerBase(const FObjectInitializer& ObjectInitializer);

    UShapeComponent* GetCollisionComponent() const { return CollisionComponent; }
};

}
