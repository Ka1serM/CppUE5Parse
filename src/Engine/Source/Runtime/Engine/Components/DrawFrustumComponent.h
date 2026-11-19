#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "DrawFrustumComponent.generated.h"

namespace ue5parse::runtime {

class UTexture;

UCLASS()
class UDrawFrustumComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bFrustumEnabled = true;
    UPROPERTY()
    FColor FrustumColor = FColor(255, 0, 255, 255);
    UPROPERTY()
    float FrustumAngle = 90.0f;
    UPROPERTY()
    float FrustumAspectRatio = 1.333333f;
    UPROPERTY()
    float FrustumStartDist = 100.0f;
    UPROPERTY()
    float FrustumEndDist = 1000.0f;
    UPROPERTY()
    TObjectPtr<UTexture> Texture = nullptr;

    explicit UDrawFrustumComponent(const FObjectInitializer& ObjectInitializer);
};

}
