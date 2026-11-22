#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "SubDSurfaceActor.generated.h"

namespace ue5parse::runtime {

class USubDSurfaceComponent;
class UStaticMeshComponent;

UCLASS()
class ASubDSurfaceActor : public AActor {
    GENERATED_BODY()
public:
    explicit ASubDSurfaceActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USubDSurfaceComponent> SubDSurface = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> DisplayMeshComponent = nullptr;
};

}
