#include "Engine/Source/Runtime/Engine/Components/SceneCaptureComponentCube.h"

namespace ue5parse::runtime {

USceneCaptureComponentCube::USceneCaptureComponentCube(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAutoActivate = true;
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_DuringPhysics;
    PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
    bTickInEditor = true;
    PostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
    PostProcessSettings.ReflectionMethod = EReflectionMethod::None;
    PostProcessSettings.LumenSurfaceCacheResolution = 0.5f;
    PostProcessSettings.VignetteIntensity = 0.0f;
    PostProcessSettings.LumenReflectionsScreenTraces = 0;
    PostProcessSettings.LumenFinalGatherScreenTraces = 0;
}

}
