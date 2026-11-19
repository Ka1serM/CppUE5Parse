#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "SceneCapture.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS(Abstract)
class ASceneCapture : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> MeshComp_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneComponent = nullptr;

    explicit ASceneCapture(const FObjectInitializer& ObjectInitializer);
};

}
