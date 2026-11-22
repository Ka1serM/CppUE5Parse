#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "MediaPlane.generated.h"

namespace ue5parse::runtime {

class UMediaPlaneComponent;

UCLASS()
class AMediaPlane : public AActor {
    GENERATED_BODY()
public:
    explicit AMediaPlane(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UMediaPlaneComponent> MediaPlane = nullptr;
};

}
