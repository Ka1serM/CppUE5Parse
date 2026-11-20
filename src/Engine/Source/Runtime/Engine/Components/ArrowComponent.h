#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "ArrowComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UArrowComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FColor ArrowColor = FColor(255, 0, 0, 255);
    UPROPERTY()
    float ArrowSize = 1.0f;
    UPROPERTY()
    float ArrowLength = 80.0f;
    UPROPERTY()
    float ScreenSize = 0.0025f;
    UPROPERTY()
    uint8 bIsScreenSizeScaled : 1 = false;
    UPROPERTY()
    uint8 bTreatAsASprite : 1 = false;

    explicit UArrowComponent(const FObjectInitializer& ObjectInitializer);
};

}
