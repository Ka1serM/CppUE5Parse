#pragma once

#include "Engine/Source/Runtime/Engine/Components/ReflectionCaptureComponent.h"

#include "PlaneReflectionCaptureComponent.generated.h"

namespace ue5parse::runtime {

class UDrawSphereComponent;
class UBoxComponent;

UCLASS()
class UPlaneReflectionCaptureComponent : public UReflectionCaptureComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float InfluenceRadiusScale = 2.0f;
    UPROPERTY()
    TObjectPtr<UDrawSphereComponent> PreviewInfluenceRadius = nullptr;
    UPROPERTY()
    TObjectPtr<UBoxComponent> PreviewCaptureBox = nullptr;
};

}
