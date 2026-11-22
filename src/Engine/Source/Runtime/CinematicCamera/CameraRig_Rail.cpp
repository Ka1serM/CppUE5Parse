#include "Engine/Source/Runtime/CinematicCamera/CameraRig_Rail.h"

#include "Engine/Source/Runtime/Engine/Components/SplineComponent.h"

namespace ue5parse::runtime {

ACameraRig_Rail::ACameraRig_Rail(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    TransformComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("TransformComponent"));
    RootComponent = TransformComponent;
    RailSplineComponent = ObjectInitializer.CreateDefaultSubobject<USplineComponent>(FName("RailSplineComponent"));
    RailSplineComponent->SetupAttachment(TransformComponent);
    RailCameraMount = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("RailCameraMount"));
    RailCameraMount->SetupAttachment(RailSplineComponent);
}

}
