#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "ReflectionCapture.generated.h"

namespace ue5parse::runtime {

class UReflectionCaptureComponent;

UCLASS(Abstract)
class AReflectionCapture : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UReflectionCaptureComponent> CaptureComponent = nullptr;

    explicit AReflectionCapture(const FObjectInitializer& ObjectInitializer);

    UReflectionCaptureComponent* GetCaptureComponent() const { return CaptureComponent; }
};

}
