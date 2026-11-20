#include "Engine/Source/Developer/FunctionalTesting/ScreenshotFunctionalTestBase.h"

#include "Engine/Source/Runtime/Engine/Camera/CameraComponent.h"

namespace ue5parse::runtime {

AScreenshotFunctionalTestBase::AScreenshotFunctionalTestBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ScreenshotCamera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(FName("Camera"));
    ScreenshotCamera->SetupAttachment(RootComponent);
}

}
