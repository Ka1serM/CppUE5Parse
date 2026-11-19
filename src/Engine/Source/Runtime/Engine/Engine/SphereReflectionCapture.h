#pragma once

#include "Engine/Source/Runtime/Engine/Engine/ReflectionCapture.h"

#include "SphereReflectionCapture.generated.h"

namespace ue5parse::runtime {

class UDrawSphereComponent;

UCLASS()
class ASphereReflectionCapture : public AReflectionCapture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UDrawSphereComponent> DrawCaptureRadius = nullptr;

    explicit ASphereReflectionCapture(const FObjectInitializer& ObjectInitializer);
};

}
