#include "Engine/Source/Runtime/CinematicCamera/CineCameraActor.h"

#include "Engine/Source/Runtime/CinematicCamera/CineCameraComponent.h"

namespace ue5parse::runtime {

ACineCameraActor::ACineCameraActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UCineCameraComponent>(FName("CameraComponent"))) {
    CineCameraComponent = Cast<UCineCameraComponent>(CameraComponent.Get());
}

}
