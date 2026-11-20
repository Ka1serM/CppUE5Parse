#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "SpringArmComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USpringArmComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit USpringArmComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
