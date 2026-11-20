#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GeometryCacheActor.generated.h"

namespace ue5parse::runtime {

class UGeometryCacheComponent;

UCLASS()
class AGeometryCacheActor : public AActor {
    GENERATED_BODY()
public:
    explicit AGeometryCacheActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UGeometryCacheComponent> GeometryCacheComponent = nullptr;
};

}
