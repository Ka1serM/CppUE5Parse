#include "Engine/Source/Runtime/Engine/Engine/SceneCaptureCube.h"

#include "Engine/Source/Runtime/Engine/Components/SceneCaptureComponentCube.h"

namespace ue5parse::runtime {

ASceneCaptureCube::ASceneCaptureCube(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    CaptureComponentCube = ObjectInitializer.CreateDefaultSubobject<USceneCaptureComponentCube>(FName("NewSceneCaptureComponentCube"));
    CaptureComponentCube->SetupAttachment(RootComponent);
}

}
