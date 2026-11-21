#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "CameraRig_Rail.generated.h"

namespace ue5parse::runtime {

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class ACameraRig_Rail : public AActor {
    GENERATED_BODY()
public:
    explicit ACameraRig_Rail(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USceneComponent> TransformComponent = nullptr;
    UPROPERTY()
    TObjectPtr<USplineComponent> RailSplineComponent = nullptr;
    UPROPERTY()
    TObjectPtr<USceneComponent> RailCameraMount = nullptr;
    UPROPERTY()
    TObjectPtr<USplineMeshComponent> PreviewMesh_Rail = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<USplineMeshComponent>> PreviewRailMeshSegments;
    UPROPERTY()
    TObjectPtr<UStaticMesh> PreviewRailStaticMesh = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PreviewMesh_Mount = nullptr;
};

}
