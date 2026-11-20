#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/ConstraintInstance.h"

#include "PhysicsConstraintComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPhysicsConstraintComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FConstraintInstance ConstraintInstance;

    explicit UPhysicsConstraintComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
