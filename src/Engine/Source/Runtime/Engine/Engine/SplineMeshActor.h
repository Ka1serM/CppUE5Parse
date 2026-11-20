#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "SplineMeshActor.generated.h"

namespace ue5parse::runtime {

class USplineMeshComponent;

UCLASS()
class ASplineMeshActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USplineMeshComponent> SplineMeshComponent = nullptr;

    explicit ASplineMeshActor(const FObjectInitializer& ObjectInitializer);

    USplineMeshComponent* GetSplineMeshComponent() const { return SplineMeshComponent; }
};

}
