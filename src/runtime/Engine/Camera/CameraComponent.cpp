#include "ue5parse/runtime/Engine/Camera/CameraComponent.h"

// UE's Camera/CameraComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

UCameraComponent::UCameraComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAutoActivate = true;
}

// The component's place and field of view. Not ported: the pawn's control rotation (bUsePawnControlRotation acts only
// for a locally controlled pawn), the additive offset, the HMD lock, the aspect ratio and the post process settings.
void UCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) {
    DesiredView.Location = GetComponentLocation();
    DesiredView.Rotation = GetComponentRotation();
    DesiredView.FOV = FieldOfView;
}

}
