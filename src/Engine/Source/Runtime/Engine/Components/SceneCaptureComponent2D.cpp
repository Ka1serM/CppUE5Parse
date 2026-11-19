#include "Engine/Source/Runtime/Engine/Components/SceneCaptureComponent2D.h"

namespace ue5parse::runtime {

USceneCaptureComponent2D::USceneCaptureComponent2D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAutoActivate = true;
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_DuringPhysics;
    PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
    bTickInEditor = true;
    PostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
    PostProcessSettings.ReflectionMethod = EReflectionMethod::None;
    PostProcessSettings.LumenSurfaceCacheResolution = 0.5f;
    CaptureSortPriority = 1;
}

}
