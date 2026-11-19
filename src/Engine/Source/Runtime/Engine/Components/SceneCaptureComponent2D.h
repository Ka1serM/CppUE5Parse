#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneCaptureComponent.h"
#include "Engine/Source/Runtime/Engine/Camera/CameraTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"

#include "SceneCaptureComponent2D.generated.h"

namespace ue5parse::runtime {

class UTextureRenderTarget2D;

UENUM()
enum class ESceneCaptureUnlitViewmode : uint8 { Disabled, Capture, CaptureOrCustomRenderPass };

UCLASS()
class USceneCaptureComponent2D : public USceneCaptureComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<ECameraProjectionMode::Type> ProjectionType = ECameraProjectionMode::Perspective;
    UPROPERTY()
    float FOVAngle = 90.0f;
    UPROPERTY()
    float FirstPersonFieldOfView = 90.0f;
    UPROPERTY()
    float FirstPersonScale = 1.0f;
    UPROPERTY()
    float OrthoWidth = 512.0f;
    UPROPERTY()
    bool bAutoCalculateOrthoPlanes = true;
    UPROPERTY()
    float AutoPlaneShift = 0.0f;
    UPROPERTY()
    bool bUpdateOrthoPlanes = false;
    UPROPERTY()
    bool bUseCameraHeightAsViewTarget = false;
    UPROPERTY()
    float Overscan = 0.0f;
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> TextureTarget = nullptr;
    UPROPERTY()
    TEnumAsByte<ESceneCaptureCompositeMode> CompositeMode = SCCM_Overwrite;
    UPROPERTY()
    FPostProcessSettings PostProcessSettings;
    UPROPERTY()
    float PostProcessBlendWeight = 1.0f;
    UPROPERTY()
    uint32 bOverride_CustomNearClippingPlane : 1 = false;
    UPROPERTY()
    float CustomNearClippingPlane = 0.0f;
    UPROPERTY()
    bool bUseCustomProjectionMatrix = false;
    UPROPERTY()
    bool bUseFauxOrthoViewPos = false;
    UPROPERTY()
    bool bEnableOrthographicTiling = false;
    UPROPERTY()
    int32 NumXTiles = 4;
    UPROPERTY()
    int32 NumYTiles = 4;
    UPROPERTY()
    FMatrix CustomProjectionMatrix = FMatrix::Identity;
    UPROPERTY()
    bool bEnableClipPlane = false;
    UPROPERTY()
    bool bRenderInMainRenderer = false;
    UPROPERTY()
    FVector ClipPlaneBase = FVector::ZeroVector;
    UPROPERTY()
    FVector ClipPlaneNormal = FVector(0, 0, 1);
    UPROPERTY()
    ESceneCaptureUnlitViewmode UnlitViewmode = ESceneCaptureUnlitViewmode::Capture;
    UPROPERTY()
    uint32 bEnableFirstPersonFieldOfView : 1 = false;
    UPROPERTY()
    uint32 bEnableFirstPersonScale : 1 = false;
    UPROPERTY()
    uint32 bConsiderUnrenderedOpaquePixelAsFullyTranslucent : 1 = false;
    UPROPERTY()
    uint32 bMainViewFamily : 1 = false;
    UPROPERTY()
    uint32 bMainViewResolution : 1 = false;
    UPROPERTY()
    uint32 bMainViewCamera : 1 = false;
    UPROPERTY()
    uint32 bInheritMainViewCameraPostProcessSettings : 1 = false;
    UPROPERTY()
    uint32 bIgnoreScreenPercentage : 1 = false;
    UPROPERTY()
    FIntPoint MainViewResolutionDivisor = FIntPoint(1, 1);
    UPROPERTY()
    FName UserSceneTextureBaseColor;
    UPROPERTY()
    FName UserSceneTextureNormal;
    UPROPERTY()
    FName UserSceneTextureSceneColor;
    UPROPERTY()
    bool bDisableFlipCopyGLES = false;

    explicit USceneCaptureComponent2D(const FObjectInitializer& ObjectInitializer);
};

}
