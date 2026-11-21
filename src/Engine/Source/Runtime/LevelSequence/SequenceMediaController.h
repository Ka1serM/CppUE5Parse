#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "SequenceMediaController.generated.h"

namespace ue5parse::runtime {

class ALevelSequenceActor;
class UMediaComponent;

UCLASS()
class ALevelSequenceMediaController : public AActor {
    GENERATED_BODY()
public:
    explicit ALevelSequenceMediaController(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<ALevelSequenceActor> Sequence = nullptr;
    UPROPERTY()
    TObjectPtr<UMediaComponent> MediaComponent = nullptr;
    UPROPERTY()
    float ServerStartTimeSeconds = 0.0f;
};

}
