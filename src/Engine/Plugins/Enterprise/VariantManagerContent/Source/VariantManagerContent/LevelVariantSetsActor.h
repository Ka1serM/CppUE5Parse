#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LevelVariantSetsActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ALevelVariantSetsActor : public AActor {
    GENERATED_BODY()
public:
    explicit ALevelVariantSetsActor(const FObjectInitializer& ObjectInitializer);
};

}
