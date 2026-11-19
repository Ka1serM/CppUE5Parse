#pragma once

#include "Engine/Source/Runtime/Engine/Components/ShapeComponent.h"

#include "CapsuleComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCapsuleComponent : public UShapeComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float CapsuleHalfHeight = 44.0f;
    UPROPERTY()
    float CapsuleRadius = 22.0f;
    UPROPERTY()
    float CapsuleHeight_DEPRECATED = 0.0f;

    explicit UCapsuleComponent(const FObjectInitializer& ObjectInitializer);

    void InitCapsuleSize(float InRadius, float InHalfHeight) {
        CapsuleRadius = std::max(0.0f, InRadius);
        CapsuleHalfHeight = std::max(CapsuleRadius, InHalfHeight);
    }
};

}
