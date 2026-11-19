#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneCaptureComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"

#include "SceneCaptureComponentCube.generated.h"

namespace ue5parse::runtime {

class UTextureRenderTargetCube;
class UTextureRenderTarget2D;

UCLASS()
class USceneCaptureComponentCube : public USceneCaptureComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UTextureRenderTargetCube> TextureTarget = nullptr;
    UPROPERTY()
    bool bCaptureRotation = false;
    UPROPERTY()
    TObjectPtr<UTextureRenderTargetCube> TextureTargetLeft = nullptr;
    UPROPERTY()
    TObjectPtr<UTextureRenderTargetCube> TextureTargetRight = nullptr;
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> TextureTargetODS = nullptr;
    UPROPERTY()
    float IPD = 6.2f;
    UPROPERTY()
    FPostProcessSettings PostProcessSettings;
    UPROPERTY()
    float PostProcessBlendWeight = 1.0f;

    explicit USceneCaptureComponentCube(const FObjectInitializer& ObjectInitializer);
};

}
