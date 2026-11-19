#pragma once

#include "Engine/Source/Runtime/Engine/Camera/CameraTypes.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"

#include "CameraComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCameraComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UCameraComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float FieldOfView = 90.0f;
    UPROPERTY()
    float FirstPersonFieldOfView = 90.0f;
    UPROPERTY()
    float FirstPersonScale = 1.0f;
    UPROPERTY()
    float OrthoWidth = 1536.0f;
    UPROPERTY()
    bool bAutoCalculateOrthoPlanes = true;
    UPROPERTY()
    float AutoPlaneShift = 0.0f;
    UPROPERTY()
    float OrthoNearClipPlane = -1536.0f / 2.0f;
    UPROPERTY()
    float OrthoFarClipPlane = 2097152.0f;
    UPROPERTY()
    bool bUpdateOrthoPlanes = true;
    UPROPERTY()
    bool bUseCameraHeightAsViewTarget = true;
    UPROPERTY()
    float AspectRatio = 1.777778f;
    UPROPERTY()
    TEnumAsByte<EAspectRatioAxisConstraint> AspectRatioAxisConstraint = AspectRatio_MaintainXFOV;
    UPROPERTY()
    uint8 bConstrainAspectRatio : 1 = false;
    UPROPERTY()
    uint8 bOverrideAspectRatioAxisConstraint : 1 = false;
    UPROPERTY()
    uint8 bUseFieldOfViewForLOD : 1 = true;
    UPROPERTY()
    float Overscan = 0.0f;
    UPROPERTY()
    FVector4f AsymmetricOverscan = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
    UPROPERTY()
    bool bScaleResolutionWithOverscan = false;
    UPROPERTY()
    bool bCropOverscan = false;
    UPROPERTY()
    uint8 bLockToHmd : 1 = true;
    UPROPERTY()
    uint8 bUsePawnControlRotation : 1 = false;
    UPROPERTY()
    uint8 bEnableFirstPersonFieldOfView : 1 = false;
    UPROPERTY()
    uint8 bEnableFirstPersonScale : 1 = false;
    UPROPERTY()
    TEnumAsByte<ECameraProjectionMode::Type> ProjectionMode = ECameraProjectionMode::Perspective;
    UPROPERTY()
    float PostProcessBlendWeight = 1.0f;
    UPROPERTY()
    FPostProcessSettings PostProcessSettings;
    UPROPERTY()
    uint8 bUseControllerViewRotation_DEPRECATED : 1 = true;

    UFUNCTION(BlueprintCallable)
    virtual void SetFieldOfView(float InFieldOfView) { FieldOfView = InFieldOfView; }
    virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView);
};

}
