#pragma once

#include "Engine/Source/Runtime/Engine/Engine/SceneCapture.h"

#include "SceneCaptureCube.generated.h"

namespace ue5parse::runtime {

class UDrawFrustumComponent;
class USceneCaptureComponentCube;

UCLASS()
class ASceneCaptureCube : public ASceneCapture {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USceneCaptureComponentCube> CaptureComponentCube = nullptr;
    UPROPERTY()
    TObjectPtr<UDrawFrustumComponent> DrawFrustum = nullptr;

    explicit ASceneCaptureCube(const FObjectInitializer& ObjectInitializer);
};

}
