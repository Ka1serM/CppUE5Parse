#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "BillboardComponent.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

UCLASS()
class UBillboardComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UTexture2D> Sprite = nullptr;
    UPROPERTY()
    uint32 bIsScreenSizeScaled : 1 = false;
    UPROPERTY()
    float ScreenSize = 0.0025f;
    UPROPERTY()
    float U = 0.0f;
    UPROPERTY()
    float UL = 0.0f;
    UPROPERTY()
    float V = 0.0f;
    UPROPERTY()
    float VL = 0.0f;
    UPROPERTY()
    float OpacityMaskRefVal = 0.5f;

    explicit UBillboardComponent(const FObjectInitializer& ObjectInitializer);
};

}
