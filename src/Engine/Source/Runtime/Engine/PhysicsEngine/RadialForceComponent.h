#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "RadialForceComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URadialForceComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit URadialForceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
