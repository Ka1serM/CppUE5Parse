#include "Engine/Source/Runtime/Engine/Camera/CameraActor.h"

#include "Engine/Source/Runtime/Engine/Camera/CameraComponent.h"

namespace ue5parse::runtime {

ACameraActor::ACameraActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComponent"));
    RootComponent = SceneComponent;
    CameraComponent = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(FName("CameraComponent"));
    CameraComponent->FieldOfView = 90.0f;
    CameraComponent->bConstrainAspectRatio = true;
    CameraComponent->AspectRatio = 1.777778f;
    CameraComponent->PostProcessBlendWeight = 1.0f;
    CameraComponent->SetupAttachment(SceneComponent);
}

}
