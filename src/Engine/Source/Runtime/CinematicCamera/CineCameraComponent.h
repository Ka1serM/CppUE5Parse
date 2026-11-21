#pragma once

#include "Engine/Source/Runtime/CinematicCamera/CineCameraSettings.h"
#include "Engine/Source/Runtime/Engine/Camera/CameraComponent.h"

#include "CineCameraComponent.generated.h"

namespace ue5parse::runtime {

class UMaterial;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class UCineCameraComponent : public UCameraComponent {
    GENERATED_BODY()
public:
    explicit UCineCameraComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FCameraFilmbackSettings FilmbackSettings_DEPRECATED;
    UPROPERTY()
    FCameraFilmbackSettings Filmback;
    UPROPERTY()
    FCameraLensSettings LensSettings;
    UPROPERTY()
    FCameraFocusSettings FocusSettings;
    UPROPERTY()
    FPlateCropSettings CropSettings;
    UPROPERTY()
    float CurrentFocalLength = 35.0f;
    UPROPERTY()
    float CurrentAperture = 2.8f;
    UPROPERTY()
    float CurrentFocusDistance = 0.0f;
    UPROPERTY()
    uint32 bOverride_CustomNearClippingPlane : 1 = false;
    UPROPERTY()
    float CustomNearClippingPlane = 0.0f;
    UPROPERTY()
    float CurrentHorizontalFOV = 0.0f;
    UPROPERTY()
    ECameraExposureMethod ExposureMethod = ECameraExposureMethod::DoNotOverride;
    UPROPERTY()
    TArray<FNamedFilmbackPreset> FilmbackPresets;
    UPROPERTY()
    TArray<FNamedLensPreset> LensPresets;
    UPROPERTY()
    FString DefaultFilmbackPresetName_DEPRECATED;
    UPROPERTY()
    FString DefaultFilmbackPreset;
    UPROPERTY()
    FString DefaultLensPresetName;
    UPROPERTY()
    float DefaultLensFocalLength = 0.0f;
    UPROPERTY()
    float DefaultLensFStop = 0.0f;
    UPROPERTY()
    TObjectPtr<UStaticMesh> FocusPlaneVisualizationMesh = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterial> FocusPlaneVisualizationMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> DebugFocusPlaneComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> DebugFocusPlaneMID = nullptr;
};

}
