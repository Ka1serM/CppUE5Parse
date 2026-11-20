#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "AmbientSound.generated.h"

namespace ue5parse::runtime {

class UAudioComponent;

UCLASS()
class AAmbientSound : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UAudioComponent> AudioComponent = nullptr;

    explicit AAmbientSound(const FObjectInitializer& ObjectInitializer);
};

}
