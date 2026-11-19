#pragma once

#include "Engine/Source/Runtime/Engine/Engine/SceneCapture.h"

#include "SceneCapture2D.generated.h"

namespace ue5parse::runtime {

class UDrawFrustumComponent;
class USceneCaptureComponent2D;

UCLASS()
class ASceneCapture2D : public ASceneCapture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USceneCaptureComponent2D> CaptureComponent2D = nullptr;
    UPROPERTY()
    TObjectPtr<UDrawFrustumComponent> DrawFrustum = nullptr;

    explicit ASceneCapture2D(const FObjectInitializer& ObjectInitializer);
};

}
