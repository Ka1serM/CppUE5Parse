#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "VectorFieldVolume.generated.h"

namespace ue5parse::runtime {

class UVectorFieldComponent;

UCLASS()
class AVectorFieldVolume : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UVectorFieldComponent> VectorFieldComponent = nullptr;

    explicit AVectorFieldVolume(const FObjectInitializer& ObjectInitializer);
};

}
