#pragma once

#include "Engine/Source/Runtime/NavigationSystem/NavModifierComponent.h"

#include "SplineNavModifierComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USplineNavModifierComponent : public UNavModifierComponent {
    GENERATED_BODY()
public:
    explicit USplineNavModifierComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
