#pragma once

#include "ue5parse/runtime/Engine/Camera/CameraTypes.h"
#include "ue5parse/runtime/Engine/Components/SceneComponent.h"

#include "CameraComponent.generated.h"

namespace ue5parse::runtime {

// UCameraComponent: a view point and its field of view (UE's Camera/CameraComponent.h). An actor that is a view
// target takes the view of its first active camera (AActor::CalcCamera).
UCLASS()
class UCameraComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UCameraComponent(const FObjectInitializer& ObjectInitializer);

    // The horizontal field of view in degrees.
    UPROPERTY()
    float FieldOfView = 90.0f;

    UFUNCTION(BlueprintCallable)
    virtual void SetFieldOfView(float InFieldOfView) { FieldOfView = InFieldOfView; }
    virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView);
};

}
