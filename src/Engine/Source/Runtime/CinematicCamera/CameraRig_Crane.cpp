#include "Engine/Source/Runtime/CinematicCamera/CameraRig_Crane.h"

namespace ue5parse::runtime {

ACameraRig_Crane::ACameraRig_Crane(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    TransformComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("TransformComponent"));
    RootComponent = TransformComponent;
    CraneYawControl = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("CraneYawControl"));
    CraneYawControl->SetupAttachment(TransformComponent);
    CranePitchControl = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("CranePitchControl"));
    CranePitchControl->SetupAttachment(CraneYawControl);
    CraneCameraMount = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("CraneCameraMount"));
    CraneCameraMount->SetupAttachment(CranePitchControl);
}

}
