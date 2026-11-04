#pragma once

#include "Engine/Source/Runtime/Engine/Camera/CameraTypes.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "CameraComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCameraComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UCameraComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float FieldOfView = 90.0f;

    UFUNCTION(BlueprintCallable)
    virtual void SetFieldOfView(float InFieldOfView) { FieldOfView = InFieldOfView; }
    virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView);
};

}
