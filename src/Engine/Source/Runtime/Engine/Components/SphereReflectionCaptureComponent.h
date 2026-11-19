#pragma once

#include "Engine/Source/Runtime/Engine/Components/ReflectionCaptureComponent.h"

#include "SphereReflectionCaptureComponent.generated.h"

namespace ue5parse::runtime {

class UDrawSphereComponent;

UCLASS()
class USphereReflectionCaptureComponent : public UReflectionCaptureComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float InfluenceRadius = 3000.0f;
    UPROPERTY()
    float CaptureDistanceScale = 1.0f;
    UPROPERTY()
    TObjectPtr<UDrawSphereComponent> PreviewInfluenceRadius = nullptr;
};

}
