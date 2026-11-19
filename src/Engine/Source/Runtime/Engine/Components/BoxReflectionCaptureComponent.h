#pragma once

#include "Engine/Source/Runtime/Engine/Components/ReflectionCaptureComponent.h"

#include "BoxReflectionCaptureComponent.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UCLASS()
class UBoxReflectionCaptureComponent : public UReflectionCaptureComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float BoxTransitionDistance = 100.0f;
    UPROPERTY()
    TObjectPtr<UBoxComponent> PreviewInfluenceBox = nullptr;
    UPROPERTY()
    TObjectPtr<UBoxComponent> PreviewCaptureBox = nullptr;
};

}
