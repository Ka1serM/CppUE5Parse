#pragma once

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "CineCameraSettings.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ECameraFocusMethod : uint8 { DoNotOverride, Manual, Tracking, Disable, MAX };

UENUM()
enum class ECameraExposureMethod : uint8 { DoNotOverride, Enabled, MAX };

USTRUCT()
struct FCameraFilmbackSettings {
    GENERATED_BODY()

    UPROPERTY()
    float SensorWidth = 24.89f;
    UPROPERTY()
    float SensorHeight = 18.67f;
    UPROPERTY()
    float SensorHorizontalOffset = 0.0f;
    UPROPERTY()
    float SensorVerticalOffset = 0.0f;
    UPROPERTY()
    float SensorAspectRatio = 1.33f;
};

USTRUCT()
struct FCameraLensSettings {
    GENERATED_BODY()

    UPROPERTY()
    float MinFocalLength = 0.0f;
    UPROPERTY()
    float MaxFocalLength = 0.0f;
    UPROPERTY()
    float MinFStop = 0.0f;
    UPROPERTY()
    float MaxFStop = 0.0f;
    UPROPERTY()
    float MinimumFocusDistance = 0.0f;
    UPROPERTY()
    float SqueezeFactor = 1.0f;
    UPROPERTY()
    int32 DiaphragmBladeCount = 0;
};

USTRUCT()
struct FPlateCropSettings {
    GENERATED_BODY()

    UPROPERTY()
    float AspectRatio = 0.0f;
};

USTRUCT()
struct FNamedFilmbackPreset {
    GENERATED_BODY()

    UPROPERTY()
    FString Name;
    UPROPERTY()
    FText DisplayName;
    UPROPERTY()
    FCameraFilmbackSettings FilmbackSettings;
};

USTRUCT()
struct FNamedLensPreset {
    GENERATED_BODY()

    UPROPERTY()
    FString Name;
    UPROPERTY()
    FCameraLensSettings LensSettings;
};

USTRUCT()
struct FCameraTrackingFocusSettings {
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<AActor> ActorToTrack;
    UPROPERTY()
    FVector RelativeOffset{0.0, 0.0, 0.0};
    UPROPERTY()
    uint8 bDrawDebugTrackingFocusPoint : 1 = false;
};

USTRUCT()
struct FCameraFocusSettings {
    GENERATED_BODY()

    UPROPERTY()
    ECameraFocusMethod FocusMethod = ECameraFocusMethod::Manual;
    UPROPERTY()
    float ManualFocusDistance = 100000.0f;
    UPROPERTY()
    FCameraTrackingFocusSettings TrackingFocusSettings;
    UPROPERTY()
    uint8 bDrawDebugFocusPlane : 1 = false;
    UPROPERTY()
    FColor DebugFocusPlaneColor{102, 26, 204, 153};
    UPROPERTY()
    uint8 bSmoothFocusChanges : 1 = false;
    UPROPERTY()
    float FocusSmoothingInterpSpeed = 8.0f;
    UPROPERTY()
    float FocusOffset = 0.0f;
};

}
