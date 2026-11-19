#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Note.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ANote : public AActor {
    GENERATED_BODY()
public:
    explicit ANote(const FObjectInitializer& ObjectInitializer);
};

}
