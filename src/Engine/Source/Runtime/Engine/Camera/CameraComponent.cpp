#include "Engine/Source/Runtime/Engine/Camera/CameraComponent.h"

namespace ue5parse::runtime {

UCameraComponent::UCameraComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAutoActivate = true;
}

void UCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) {
    DesiredView.Location = GetComponentLocation();
    DesiredView.Rotation = GetComponentRotation();
    DesiredView.FOV = FieldOfView;
}

}
