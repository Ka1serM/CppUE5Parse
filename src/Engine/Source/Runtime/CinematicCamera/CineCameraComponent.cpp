#include "Engine/Source/Runtime/CinematicCamera/CineCameraComponent.h"

namespace ue5parse::runtime {

//! The defaults are the "16:9 Digital Film" and "Universal Zoom" presets of BaseEngine.ini.
UCineCameraComponent::UCineCameraComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    bConstrainAspectRatio = true;
    Filmback.SensorWidth = 23.76f;
    Filmback.SensorHeight = 13.365f;
    Filmback.SensorAspectRatio = Filmback.SensorWidth / Filmback.SensorHeight;
    FilmbackSettings_DEPRECATED = Filmback;
    LensSettings.MinFocalLength = 4.0f;
    LensSettings.MaxFocalLength = 1000.0f;
    LensSettings.MinFStop = 1.2f;
    LensSettings.MaxFStop = 22.0f;
    LensSettings.DiaphragmBladeCount = 7;
}

}
