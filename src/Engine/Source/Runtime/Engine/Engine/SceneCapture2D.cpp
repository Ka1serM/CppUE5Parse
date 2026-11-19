#include "Engine/Source/Runtime/Engine/Engine/SceneCapture2D.h"

#include "Engine/Source/Runtime/Engine/Components/SceneCaptureComponent2D.h"

namespace ue5parse::runtime {

ASceneCapture2D::ASceneCapture2D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    CaptureComponent2D = ObjectInitializer.CreateDefaultSubobject<USceneCaptureComponent2D>(FName("NewSceneCaptureComponent2D"));
    CaptureComponent2D->SetupAttachment(RootComponent);
}

}
