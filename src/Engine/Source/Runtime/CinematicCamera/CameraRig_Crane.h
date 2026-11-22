#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "CameraRig_Crane.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS()
class ACameraRig_Crane : public AActor {
    GENERATED_BODY()
public:
    explicit ACameraRig_Crane(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USceneComponent> TransformComponent = nullptr;
    UPROPERTY()
    TObjectPtr<USceneComponent> CraneYawControl = nullptr;
    UPROPERTY()
    TObjectPtr<USceneComponent> CranePitchControl = nullptr;
    UPROPERTY()
    TObjectPtr<USceneComponent> CraneCameraMount = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PreviewMesh_CraneArm = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PreviewMesh_CraneBase = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PreviewMesh_CraneMount = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PreviewMesh_CraneCounterWeight = nullptr;
};

}
