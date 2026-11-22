#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "MediaPlate.generated.h"

namespace ue5parse::runtime {

class UMediaPlateComponent;
class UStaticMeshComponent;

UCLASS()
class AMediaPlate : public AActor {
    GENERATED_BODY()
public:
    explicit AMediaPlate(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UMediaPlateComponent> MediaPlateComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
};

}
