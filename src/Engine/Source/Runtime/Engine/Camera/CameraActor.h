#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Scene.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "CameraActor.generated.h"

namespace ue5parse::runtime {

class UCameraComponent;

UCLASS()
class ACameraActor : public AActor {
    GENERATED_BODY()
public:
    explicit ACameraActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<EAutoReceiveInput::Type> AutoActivateForPlayer = EAutoReceiveInput::Disabled;
    UPROPERTY()
    TObjectPtr<UCameraComponent> CameraComponent = nullptr;
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneComponent = nullptr;
    UPROPERTY()
    uint8 bConstrainAspectRatio_DEPRECATED : 1 = true;
    UPROPERTY()
    float AspectRatio_DEPRECATED = 1.777778f;
    UPROPERTY()
    float FOVAngle_DEPRECATED = 90.0f;
    UPROPERTY()
    float PostProcessBlendWeight_DEPRECATED = 1.0f;
    UPROPERTY()
    FPostProcessSettings PostProcessSettings_DEPRECATED;
};

}
